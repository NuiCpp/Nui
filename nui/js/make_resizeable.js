// TODO: improve:

const makeResizable = (element, borderSize, edge) => {
    edge = edge || "right";

    let pos;
    const resize = (e) => {
        const w = parseInt(getComputedStyle(element, '').width);
        const h = parseInt(getComputedStyle(element, '').height);

        if (edge === "right") {
            const dx = -(pos - e.x);
            pos = e.x;
            element.style.width = (w + dx) + "px";
        }
        if (edge === "bottom") {
            const dy = -(pos - e.y);
            pos = e.y;
            element.style.height = (h + dy) + "px";
        }
    }

    element.addEventListener("mousedown", (e) => {
        const w = parseInt(getComputedStyle(e.target, '').width);
        const h = parseInt(getComputedStyle(e.target, '').height);

        if (edge == "right") {
            if (w - e.x < borderSize + 1 /*rounding tolerance*/) {
                pos = e.x;
                document.addEventListener("mousemove", resize, false);
            }
        }
        if (edge == "bottom") {
            if (h - e.y < borderSize + 1 /*rounding tolerance*/) {
                pos = e.y;
                document.addEventListener("mousemove", resize, false);
            }
        }
    }, false);

    document.addEventListener("mouseup", () => {
        document.removeEventListener("mousemove", resize, false);
    }, false);
}

export default makeResizable;