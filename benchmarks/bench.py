import sys, os
import argparse
import subprocess
import glob
from textwrap import dedent
import chevron
import json
import pickle

def generate(args):
    benchmarks = glob.glob(os.path.join(args.dir, '**.bench'))
    print(dedent(f'''\
        function(add_benchmark name benchf WHICHS NS)
            add_custom_target(buildbench-${{name}}
                COMMAND 
                    ${{BENCHMARK_PY}} ${{BENCHMARK_LIST}}/benchrunner.py
                    "$<TARGET_PROPERTY:buildbench-${{name}},BUILDBENCH_BENCHMARKS>"
                    ${{CMAKE_COMMAND}} --build ${{CMAKE_BINARY_DIR}} -j1 --target
                COMMENT "Running ${{name}}"
                VERBATIM
                USES_TERMINAL
            )

            foreach(which IN LISTS WHICHS)
                add_custom_target(buildbench-${{name}}-${{which}}
                    COMMAND "{sys.executable}" "{os.path.abspath(__file__)}" run
                        ${{name}} ${{which}} ${{benchf}}
                        --ns "${{NS}}"
                        --cmake ${{CMAKE_COMMAND}}
                        --cmake-binary-dir ${{CMAKE_BINARY_DIR}}
                        --workingdir ${{CMAKE_CURRENT_BINARY_DIR}}/buildbench
                        --generated-file ${{BenchGeneratedFile}}
                        --build-target ${{BenchCompileTarget}}
                        --clean-target ${{BenchCleanTarget}}
                    DEPENDS
                        "{os.path.abspath(__file__)}"
                        "${{benchf}}"
                    COMMENT "Running benchmark ${{name}}-${{which}}"
                    USES_TERMINAL
                )
                set_property(TARGET buildbench-${{name}} APPEND PROPERTY BUILDBENCH_BENCHMARKS buildbench-${{name}}-${{which}})
            endforeach()

            set_property(TARGET buildbench APPEND PROPERTY BUILDBENCH_BENCHMARKS buildbench-${{name}})
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${{benchf}})
        endfunction()
    '''))

    for benchmark in benchmarks:
        with open(benchmark, 'r') as f:
            firstline = f.readline()
            secondline = f.readline()

        assert firstline.startswith('//')
        assert secondline.startswith('//')
        firstline = firstline[2:]
        secondline = secondline[2:]

        name, firstline = firstline.split(':', maxsplit=1)
        whichs = [s.strip() for s in firstline.split(',')]
        whichs = ';'.join(whichs)

        ns = [s.strip() for s in secondline.split(',')]
        ns = ','.join(ns)
        print(f'add_benchmark({name.strip()} "{os.path.abspath(benchmark)}" "{whichs}" "{ns}")')


def evaluate_template(benchtempl, outputfile, config, m, n):
    context = {
        'N': [{'n': x} for x in range(n)],
        'M': [{'m': x} for x in range(m)],
        config: True,
    }
    with open(benchtempl, 'r') as f:
        result = chevron.render(f.read(), context)

    with open(outputfile, 'w') as f:
        f.write(result)


def estimate_runtime(measure, benchtempl, outputfile, config, n):
    evaluate_template(benchtempl, outputfile, config, m=3, n=n)
    baseeval = measure(3, n)
    return baseeval['time']


def run(args):
    CMAKE_COMMAND = args.cmake
    CMAKE_BINARY_DIR = args.cmake_binary_dir

    cmake_build_target = [CMAKE_COMMAND, '--build', CMAKE_BINARY_DIR, '--target', args.build_target]
    cmake_clean_target = [CMAKE_COMMAND, '--build', CMAKE_BINARY_DIR, '--target', args.clean_target]

    runner_py = os.path.abspath(os.path.join(os.path.dirname(__file__), 'runner.py'))
    runner = [sys.executable, runner_py, '--timeout', str(args.timeout), '--', *cmake_build_target]

    def measure(m, n):
        res = subprocess.run(runner, capture_output=True)
        res.check_returncode()
        subprocess.run(cmake_clean_target, stdout=subprocess.DEVNULL).check_returncode()
        results = json.loads(res.stdout)

        time = results['time'] / m
        time = time
        mem = round(results['memory'] / m)
        results = { 'm': m, 'n': n, 'time': time, 'memory': mem }

        return results

    ns = [int(x) for x in args.ns.split(',')]

    bench_results = {
        'name': args.bench,
        'which': args.which,
        'results': [],
    }

    print(dedent(f'''\
    {{
        "name": "{args.bench}",
        "which": "{args.which}",
        "results": ['''))

    for n in ns:
        esttime = estimate_runtime(measure, args.benchfile, args.generated_file, config=args.which, n=n)

        MAX_EST_TIME = 2.0
        MAX_EST_M = int(MAX_EST_TIME / esttime)
        MAX_M = 200
        MIN_M = 10

        M = max(MIN_M, min(MAX_EST_M, MAX_M))

        evaluate_template(args.benchfile, args.generated_file, config=args.which, m=M, n=n)
        results = measure(M, n)
        bench_results['results'].append(results)

        print('        ', end='')
        json.dump(results, fp=sys.stdout)
        if n != ns[-1]: print(',', end='')
        print()

    print(dedent(f'''\
        ]
    }}'''))

    BASELINE_M = 50
    evaluate_template(args.benchfile, args.generated_file, config=args.which, m=BASELINE_M, n=0)
    baseline_results = []
    for _ in range(10):
        baseline_results.append(measure(BASELINE_M, 0))

    baseline_results = {
        'm': 1,
        'n': 0,
        'time': min(x['time'] for x in baseline_results),
        'memory': min(x['memory'] for x in baseline_results),
    }
    bench_results['baseline'] = baseline_results

    cumulative_results_f = os.path.join(args.workingdir, 'bench.results.pickle')
    if os.path.exists(cumulative_results_f):
        with open(cumulative_results_f, 'rb') as f:
            cumulative_results = pickle.load(f)
    else:
        cumulative_results = dict()

    if args.bench not in cumulative_results:
        cumulative_results[args.bench] = dict()
    
    cumulative_results[args.bench][args.which] = bench_results
    with open(cumulative_results_f, 'wb') as f:
        pickle.dump(cumulative_results, file=f)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='manage the benchmarks')
    sp = parser.add_subparsers()

    generate_p = sp.add_parser('generate')
    generate_p.add_argument('dir', help='The directory containing .bench files')
    generate_p.add_argument('-o', '--output', required=True, help='Where to generate the CMakeLists.txt')
    generate_p.set_defaults(func=generate)

    run_p = sp.add_parser('run')
    run_p.add_argument('bench', help='The name of the benchmark')
    run_p.add_argument('which', help='The benchmark configuration')
    run_p.add_argument('benchfile', help='The file which constitutes the benchmark')
    run_p.add_argument('--ns', required=True, help='What N values to use')
    run_p.add_argument('--cmake', default='cmake', help='The ${CMAKE_COMMAND}')
    run_p.add_argument('--cmake-binary-dir', required=True, help='The ${CMAKE_BINARY_DIR}')
    run_p.add_argument('--workingdir', required=True, help='Where to generate the benchmark info')
    run_p.add_argument('--generated-file', required=True, help='Where to generate the benchmarks .cpp file.')
    run_p.add_argument('-o', '--output', help='The file to output to (defaults to stdout)')
    run_p.add_argument('--build-target', required=True, help='The CMake target which will run the benchmark')
    run_p.add_argument('--clean-target', required=True, help='The CMake target which will clean the benchmark')
    run_p.add_argument('--keep-temps', action='store_true', help='Whether to keep the generated .cpp files')
    run_p.add_argument('--timeout', default=100, type=int, help='The amount of seconds before timing out a benchmark')
    run_p.set_defaults(func=run)

    args = parser.parse_args()
    func = args.func
    del args.func
    if args.output is not None:
        sys.stdout = open(args.output, 'w')

    func(args)
