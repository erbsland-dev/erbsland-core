function renderMermaid(retryCount = 0) {
    if (typeof mermaid === "undefined") {
        if (retryCount < 20) {
            window.setTimeout(() => renderMermaid(retryCount + 1), 100);
        }
        return;
    }
    mermaid.initialize({startOnLoad: false});
    mermaid.run({querySelector: "div.mermaid"});
}

if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", () => renderMermaid(), {once: true});
} else {
    renderMermaid();
}
