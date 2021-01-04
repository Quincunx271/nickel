(function () {
    'use strict';

    // Replaced by a script
    const versions = [['main', 'git-main'], ['0.0.3', 'latest'], ['0.0.3', '0.0.3']]

    function createOutdatedBanner(latestVersion, currentVersion) {
        if (latestVersion === currentVersion || currentVersion === versions[0][0]) {
            return ''
        }

        const root = $('body').data('documentation-root')
        const page = $('body').data('documentation-current-page')
        var versionBase = `${root}/${versions[1][0]}`
        var redirectUrl = `${versionBase}/${page}`
        return `<div class='body' id='outdated-banner' style='background-color: khaki; text-align: center; padding: 5px; max-width: none'>
            This is documentation for an older version of Nickel.
            <a href='${redirectUrl}'>Click here to go to the latest version.</a>
        </div>`
    }

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
        const selectorHtml = createSelector(version)

        const selectorContainer = document.getElementById('version-selector-container');
        selectorContainer.innerHTML = selectorHtml

        const outdatedBannerHtml = createOutdatedBanner(versions[1][0], version)
        if (outdatedBannerHtml != '') {
            const docDiv = document.querySelector('div.document')
            const banner = new DOMParser().parseFromString(outdatedBannerHtml, 'text/html').body.firstChild
            docDiv.parentElement.insertBefore(banner, docDiv)
        }

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
