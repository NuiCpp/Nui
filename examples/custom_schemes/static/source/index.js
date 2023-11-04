(() => {
    setTimeout(() => {
        console.log('fetch.js');
        fetch('nui://host/bla.txt', {
            headers: {
                'Origin': window.location.origin
            },
        }).catch(reason => console.error('A', reason)).then(res => {
            if (res === undefined)
                return;
            console.log(res);
            res.headers.forEach((value, key) => {
                console.log(key, value);
            });
            return res.text();
        }).catch(reason => console.error('A', reason)).then(text => {
            console.log("body", text);
            // Append to body:
            const div = document.createElement('div');
            div.innerText = 'A' + text;
            document.body.appendChild(div);
        });

        fetch('nui:/bla.txt', {
        }).then(res => {
            if (res === undefined)
                return;
            console.log(res);
            res.headers.forEach((value, key) => {
                console.log(key, value);
            });
            return res.text();
        }).catch(reason => console.error('B', reason)).then(text => {
            console.log("body", text);

            // Append to body:
            const div = document.createElement('div');
            div.innerText = 'B' + text;
            document.body.appendChild(div);
        }).catch(reason => console.error('B', reason));

        fetch('assets://host/bla.txt', {}).then(res => {
            if (res === undefined)
                return;
            console.log(res);

            if (res.status !== 200) {
                console.error('C', res.status, res.statusText);
                return;
            }

            res.headers.forEach((value, key) => {
                console.log(key, value);
            });
            return res.text();
        }).catch(reason => console.error('C', reason)).then(text => {
            console.log("body", text);

            // Append to body:
            const div = document.createElement('div');
            div.innerText = 'C' + text;
            document.body.appendChild(div);
        })
    }, 1000);
})();
