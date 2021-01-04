(function () {
    'use strict';

    // Replaced by a script
    const versions = [['main', 'git-main'], ['0.0.3', 'latest'], ['0.0.3', '0.0.3']]

    function createSelector(currentVersion) {
        var generated = ['<select id="version-selector">']

        versions.forEach(([value, display]) => {
            if (value === currentVersion && currentVersion != versions[1][0]
                || currentVersion === versions[1][0] && display === 'latest') {
                generated.push(`<option value="${value}" selected>${display}</option>`)
            } else {
                generated.push(`<option value="${value}">${display}</option>`)
            }
        });

        generated.push('</select>')

        return generated.join('')
    }

    $(document).ready(function () {
        const page = $('body').data('documentation-current-page')
        const versionRoot = window.location.pathname.slice(0, -page.length - 1) // extra char == last '/'
        const version = versionRoot.substring(versionRoot.lastIndexOf('/') + 1)
        var selectorHtml = createSelector(version)

        var selectorContainer = document.getElementById('version-selector-container');
        selectorContainer.innerHTML = selectorHtml

        $(selectorContainer).ready(function () {
            var selector = document.getElementById('version-selector')
            selector.onchange = function () {
                const root = $('body').data('documentation-root')
                const page = $('body').data('documentation-current-page')
                var versionBase = `${root}/${selector.value}`
                var redirectUrl = `${versionBase}/${page}`

                if (redirectUrl != window.location.href) {
                    $.ajax(redirectUrl, {
                        success: function () { window.location.href = redirectUrl },
                        error: function () { window.location.href = `${versionBase}/index.html` }
                    })
                }
            }
        })
    })
})()
