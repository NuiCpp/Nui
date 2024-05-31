export type AnyFunction = (...args: any[]) => any;

class RpcClient {
    constructor() {
    }

    public static UnresolvedError = class {
        private message: string;
        public readonly name = "UnresolvedRemoteCallableError";

        constructor(name: string) {
            this.message = `Remote callable with name '${name}' is undefined`;
        }
    };

    private static resolve = (name: string) => {
        const rpcObject = globalThis.nui_rpc;
        if (rpcObject === undefined)
            return undefined;

        if (rpcObject.backend === undefined)
            return undefined;

        if (!rpcObject.backend.hasOwnProperty(name))
            return undefined;

        return rpcObject.backend[name];
    }

    public static getRemoteCallable(name: string) {
        return (...args: any[]) : any => {
            let resolved: AnyFunction | undefined = undefined;
            const memoize = (): AnyFunction | undefined => {
                if (resolved !== undefined)
                    return resolved;
                resolved = RpcClient.resolve(name);
                console.log(name, resolved);
                return resolved;
            };

            return memoize() ? resolved!(...args) : new RpcClient.UnresolvedError(name);
        }
    }

    public static getRemoteCallableWithBackChannel(name: string, cb: AnyFunction)
    {
        return (...args: any[]) : any => {
            const tempId = globalThis.nui_rpc.tempId + 1;
            globalThis.nui_rpc.tempId = tempId;

            const tempIdString = `temp_${tempId}`;
            globalThis.nui_rpc.backend[tempIdString] = (param: any) => {
                cb(param);
                delete globalThis.nui_rpc.backend[tempIdString];
            };

            const resolved = RpcClient.resolve(name);
            if (resolved === undefined)
                return new RpcClient.UnresolvedError(name);
            return resolved(tempIdString, ...args);
        }
    }

    public static call(name: string, ...args: any[]) {
        if (args.length > 0 && typeof args[0] === 'function') {
            const cb = args[0];
            const restArgs = args.slice(1);
            const callable = RpcClient.getRemoteCallableWithBackChannel(name, cb);
            if (callable instanceof RpcClient.UnresolvedError)
                return callable;
            return callable(...restArgs);
        } else {
            const callable = RpcClient.getRemoteCallable(name);
            if (callable instanceof RpcClient.UnresolvedError)
                return callable;
            return callable(...args);
        }
    }

    // Only use for functions that respond via callback
    public static callAsync(name: string, ...args: any[]): Promise<any> {
        return new Promise((resolve, reject) => {
            const callback = (result: any) => {
                resolve(result);
            };

            const callable = RpcClient.getRemoteCallableWithBackChannel(name, callback);
            if (callable instanceof RpcClient.UnresolvedError) {
                reject(callable);
            } else {
                const result = callable(...args);
                if (result instanceof RpcClient.UnresolvedError) {
                    reject(result);
                }
            }
        });
    }

    public static register(name: string, func: AnyFunction) {
        globalThis.nui_rpc.frontend[name] = func;
    }

    public static unregister(name: string) {
        delete globalThis.nui_rpc.frontend[name];
    }

    public static registerMany(funcs: { [key: string]: AnyFunction }) {
        for (const key in funcs) {
            RpcClient.register(key, funcs[key]);
        }
    }

    public static unregisterMany(names: string[]) {
        for (const name of names) {
            RpcClient.unregister(name);
        }
    }
}

export default RpcClient;