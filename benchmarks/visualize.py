from matplotlib import pyplot as plt
import pickle

import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('results', help='The results file to load')

    args = parser.parse_args()

    with open(args.results, 'rb') as f:
        results = pickle.load(f)

    for benchmark, bench_results in results.items():
        plt.ylabel('Time (ms)')
        plt.xlabel('N')

        for which, which_results in bench_results.items():
            plt.plot([x['n'] for x in which_results['results']], [(x['time'] - which_results['baseline']['time']) * 1e3 for x in which_results['results']], label=which_results['which'])

        plt.show()
