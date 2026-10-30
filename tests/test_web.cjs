const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const test = require('node:test');

test('browser releases language and token response buffers after reading them', async () => {
    const values = new Map();
    const freed = [];
    let next = 1;
    const allocate = text => { const ptr = next++; values.set(ptr, text); return ptr; };
    const handlers = {};
    const elements = {};
    function element(id) {
        if (!elements[id]) elements[id] = {
            value: id === 'source' ? 'int x;' : 'cpp',
            options: [], selectedIndex: 0, style: {},
            appendChild(child) { if (id === 'language') this.options.push(child); },
            addEventListener(name, callback) { handlers[id + ':' + name] = callback; },
        };
        return elements[id];
    }
    const module = {
        _initModule() {},
        cwrap(name, type) {
            return () => {
                const value = name === 'getLanguageNames'
                    ? '[{"id":"cpp","name":"C++"}]'
                    : '{"tokens":[{"type":"KEYWORD","lexeme":"int","line":1,"column":1}]}';
                const ptr = allocate(value);
                return type === 'string' ? value : ptr;
            };
        },
        UTF8ToString(ptr) { return values.get(ptr); },
        _free(ptr) { assert.ok(values.delete(ptr)); freed.push(ptr); },
    };
    const context = {
        LexModule: () => Promise.resolve(module),
        document: { getElementById: element, createElement: () => element(Symbol()) },
        window: { addEventListener() {} },
        console: { log() {}, warn() {}, error() {} },
        performance: { now: () => 1 },
        setTimeout: callback => callback(),
    };
    vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../web/app.js'), 'utf8'), context);
    await new Promise(resolve => setImmediate(resolve));
    assert.equal(freed.length, 1, 'language response must be released');
    handlers['analyzeBtn:click']();
    assert.equal(freed.length, 2, 'token response must be released');
    assert.equal(values.size, 0);
    let prevented = false;
    handlers['source:keydown']({key: 'Enter', ctrlKey: true, metaKey: false, preventDefault() { prevented = true; }});
    assert.equal(prevented, true);
    assert.equal(freed.length, 3, 'keyboard shortcut must run analysis');
    handlers['source:keydown']({key: 'Enter', ctrlKey: false, metaKey: true, preventDefault() {}});
    assert.equal(freed.length, 4, 'Mac keyboard shortcut must run analysis');
    element('source').value = 'int a;\0int b;';
    handlers['analyzeBtn:click']();
    assert.equal(freed.length, 4, 'input containing a null byte must not be sent to WebAssembly');
    assert.match(element('output').textContent, /null byte/);
});

test('custom language text is escaped and clearing cancels pending analysis', async () => {
    const values = new Map(), handlers = {}, elements = {}, created = [], pending = [];
    let next = 1, calls = 0;
    const id = '<img src=x onerror=alert(1)>';
    const name = '<svg onload=alert(2)>';
    function element(key) {
        if (!elements[key]) elements[key] = {
            value: key === 'source' ? 'hello' : id,
            options: [], selectedIndex: 0, style: {}, children: [],
            appendChild(child) { this.children.push(child); if (key === 'language') this.options.push(child); },
            addEventListener(event, callback) { handlers[key + ':' + event] = callback; },
            get text() { return this.textContent; },
        };
        return elements[key];
    }
    const module = {
        _initModule() {},
        cwrap(call) { return () => {
            const value = call === 'getLanguageNames' ? JSON.stringify([{id, name}]) : (calls++, '{"tokens":[]}');
            const ptr = next++; values.set(ptr, value); return ptr;
        }; },
        UTF8ToString(ptr) { return values.get(ptr); },
        _free(ptr) { values.delete(ptr); },
    };
    const context = {
        LexModule: () => Promise.resolve(module),
        document: { getElementById: element, createElement: () => { const e = element(Symbol()); created.push(e); return e; } },
        window: { addEventListener() {} }, console: { log() {}, warn() {}, error() {} },
        performance: { now: () => 1 }, setTimeout: callback => pending.push(callback),
    };
    vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../web/app.js'), 'utf8'), context);
    await new Promise(resolve => setImmediate(resolve));
    handlers['analyzeBtn:click']();
    handlers['clearBtn:click']();
    pending.shift()();
    assert.equal(calls, 0);
    assert.equal(element('output').innerHTML, '');
    assert.equal(element('loading').style.display, 'none');
    element('source').value = 'hello';
    handlers['analyzeBtn:click']();
    element('language').options.push({text: 'Other'});
    element('language').selectedIndex = 1;
    element('language').value = 'other';
    pending.shift()();
    const info = created.find(e => e.className === 'language-info');
    assert.match(info.innerHTML, /&lt;svg/);
    assert.match(info.innerHTML, /&lt;img/);
    assert.doesNotMatch(info.innerHTML, /<svg|<img/);
});
