const makeResizeable = (element: HTMLElement, borderSize: number, edge?: "right" | "bottom") => {
    edge = edge || "right";

    let pos: number;
    const resizer = (() => {
        if (edge === "right") {
            return (x: number, y: number, w: number, h: number) => {
                const dx = -(pos - x);
                pos = x;
                element.style.width = (w + dx) + "px";       
            };
        }
        else if (edge === "bottom") {
            return (x: number, y: number, w: number, h: number) => {
                const dy = -(pos - y);
                pos = y;
                element.style.height = (h + dy) + "px";
            };
        }
        else {
            throw new Error("Invalid edge");
        }
    })();

    const resize = (e: MouseEvent) => {
        const w = parseInt(getComputedStyle(element, '').width);
        const h = parseInt(getComputedStyle(element, '').height);

        resizer(e.x, e.y, w, h);
    }

    element.addEventListener("mousedown", (e: MouseEvent) => {
        if (!(e.target instanceof Element))
            return;

        const w = parseInt(getComputedStyle(e.target as Element, '').width);
        const h = parseInt(getComputedStyle(e.target as Element, '').height);

        if (edge == "right") {
            if (w - e.x < borderSize + 1 /*rounding tolerance*/) {
                pos = e.x;
                document.addEventListener("mousemove", resize, false);
            }
        }
        else if (edge == "bottom") {
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

!('nui_lib' in globalThis) && (globalThis.nui_lib = {});
globalThis.nui_lib.makeResizeable = makeResizeable;

export default makeResizeable;