(function () {
    'use strict';

    // Replaced by a script
    const versions = [['main', 'git-main'], ['0.0.3', 'latest'], ['0.0.3', '0.0.3']]

    function createSelector() {
        var generated = ['<select id="version-selector">']

        versions.forEach(([value, display]) => {
            generated.push(`<option value="${value}">${display}</option>`)
        });

        generated.push('</select>')

        return generated.join('')
    }

    $(document).ready(function () {
        var selectorHtml = createSelector()
        var selectorContainer = document.getElementById('version-selector-container');
        selectorContainer.innerHTML = selectorHtml

        $(selectorContainer).ready(function () {
            var selector = document.getElementById('version-selector')
            selector.onchange = function () {
                var root = $('body').data('documentation-root')
                var page = $('body').data('documentation-current-page')
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
