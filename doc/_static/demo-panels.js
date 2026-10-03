/*
 * Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
 * SPDX-License-Identifier: Apache-2.0
 */

// Expand and collapse a panel's source code (by clicking on its link).
// Move each unique source between its (hidden) appendix and the active panel.
document.addEventListener('DOMContentLoaded', () => {

    const appendix = document.querySelector('.demo-source-appendix');
    if (!appendix) return;

    function updateButton(buttonDiv, isOpen) {
        buttonDiv.textContent = "";
        const icon = document.createElement('i');
        if (isOpen) {
            buttonDiv.setAttribute('aria-expanded', 'true');
            buttonDiv.title = 'Collapse the full listing.';
            icon.className = 'fa-solid fa-down-left-and-up-right-to-center';
        } else {
            buttonDiv.setAttribute('aria-expanded', 'false');
            buttonDiv.title = 'Expand the full listing.'
            icon.className = 'fa-solid fa-up-right-and-down-left-from-center';
        }
        buttonDiv.append(icon);
    }

    document.querySelectorAll('.demo-source-link').forEach(link => {
        const source = document.getElementById(link.hash.slice(1));
        if (!source) return;
        const sourceCode = source.querySelector('div');
        const panel = link.closest('.erbsland-demo');
        if (!panel) return;
        const demoExcerptContainer = panel.querySelector('.demo-excerpt');
        const collapsedCode = demoExcerptContainer.querySelector('div');

        const button = document.createElement('div');
        button.className = 'demo-source-toggle';
        button.setAttribute('aria-controls', source.id);
        button.dataset.state = "closed";
        updateButton(button, false);

        button.addEventListener('click', () => {
            const isOpen = button.dataset.state !== "closed";
            updateButton(button, !isOpen);
            if (isOpen) {
                button.dataset.state = "closed";
                collapsedCode.classList.remove('erbsland-demo-hidden');
                appendix.append(sourceCode);
            } else {
                button.dataset.state = "open";
                collapsedCode.classList.add('erbsland-demo-hidden');
                demoExcerptContainer.append(sourceCode);
            }
        });
        link.replaceWith(button);
    });

    appendix.hidden = true;
});
