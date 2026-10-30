const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const web = path.join(__dirname, '..', 'web');
const init = require(path.join(web, 'lex.js'));
const data = fs.readFileSync(path.join(web, 'lex.data'));

init({
    wasmBinary: fs.readFileSync(path.join(web, 'lex.wasm')),
    getPreloadedPackage: () => data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength),
    print() {},
    printErr() {},
}).then(module => {
    const tokenize = module.cwrap('tokenizeString', 'number', ['string', 'string']);
    const errorPointer = tokenize('int x;', 'missing');
    assert.ok(errorPointer);
    try {
        const result = JSON.parse(module.UTF8ToString(errorPointer));
        assert.match(result.error, /No language plugins available/);
        assert.deepEqual(result.tokens, []);
    } finally {
        module._free(errorPointer);
    }
    module._initModule();
    const pointer = tokenize('int x = 0b101;', 'cpp');
    assert.ok(pointer);
    try {
        const result = JSON.parse(module.UTF8ToString(pointer));
        assert.ok(result.tokens.some(token => token.type === 'BINARY' && token.lexeme === '0b101'));
    } finally {
        module._free(pointer);
    }
    for (const [language, source, expected] of [
        ['c', "'x'", 'CHAR_LITERAL'],
        ['java', "'x'", 'CHAR_LITERAL'],
        ['python', '"""hello"""', 'STRING_LITERAL'],
    ]) {
        const resultPointer = tokenize(source, language);
        assert.ok(resultPointer);
        try {
            const result = JSON.parse(module.UTF8ToString(resultPointer));
            assert.equal(result.tokens.length, 1);
            assert.equal(result.tokens[0].type, expected);
            assert.equal(result.tokens[0].lexeme, language === 'python' ? 'hello' : 'x');
        } finally {
            module._free(resultPointer);
        }
    }
    for (const [language, source, type, lexeme] of [
        ['cpp', 'R"tag(a"b)tag"', 'STRING_LITERAL', 'a"b'],
        ['cpp', '1.0f', 'FLOAT', '1.0f'],
        ['c', '1.0f', 'FLOAT', '1.0f'],
        ['c', '.5', 'FLOAT', '.5'],
        ['c', '1.', 'FLOAT', '1.'],
        ['c', '1e3f', 'SCIENTIFIC', '1e3f'],
        ['python', 'R"hello"', 'STRING_LITERAL', 'hello'],
        ['js', '?.', 'OPERATOR', '?.'],
        ['java', '1e3', 'SCIENTIFIC', '1e3'],
        ['python', 'f"hello {name}"', 'STRING_LITERAL', 'hello {name}'],
        ['python', ':=', 'ASSIGNMENT_OPERATOR', ':='],
    ]) {
        const resultPointer = tokenize(source, language);
        assert.ok(resultPointer);
        try {
            const result = JSON.parse(module.UTF8ToString(resultPointer));
            assert.equal(result.tokens.length, 1);
            assert.equal(result.tokens[0].type, type);
            assert.equal(result.tokens[0].lexeme, lexeme);
        } finally {
            module._free(resultPointer);
        }
    }
    for (const [language, source] of [['c', '0x'], ['cpp', '1e+'], ['js', '0x']]) {
        const resultPointer = tokenize(source, language);
        try {
            const result = JSON.parse(module.UTF8ToString(resultPointer));
            assert.match(result.error, /Invalid numeric literal/);
        } finally {
            module._free(resultPointer);
        }
    }
    {
        const resultPointer = tokenize('a?.1:2', 'js');
        try {
            const result = JSON.parse(module.UTF8ToString(resultPointer));
            assert.deepEqual(result.tokens.map(token => token.lexeme), ['a', '?', '.1', ':', '2']);
        } finally {
            module._free(resultPointer);
        }
    }
    const register = module.cwrap('registerLanguageConfig', 'number', ['string', 'string']);
    assert.equal(register('invalid', '{'), 0);
    assert.equal(register('bad"id', JSON.stringify({ name: 'Test' })), 1);
    const languagePointer = module.cwrap('getLanguageNames', 'number', [])();
    try {
        const languages = JSON.parse(module.UTF8ToString(languagePointer));
        assert.ok(languages.some(language => language.id === 'bad"id'));
    } finally {
        module._free(languagePointer);
    }
    assert.equal(register('custom', JSON.stringify({ name: 'Custom', tokenRules: [
        { name: 'slash', pattern: '\\\\', type: 10, precedence: 1 },
    ] })), 1);
    for (const [id, source, type, lexeme] of [
        ['cpp', 'R"tag(a"b)tag"', 'STRING_LITERAL', 'a"b'],
        ['js', 'a===b', 'COMPARISON_OPERATOR', '==='],
        ['python', '10 // 2', 'OPERATOR', '//'],
    ]) {
        const config = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'plugins', id + '_config.json'), 'utf8'));
        config.name = 'Renamed';
        assert.equal(register('renamed-' + id, JSON.stringify(config)), 1);
        const resultPointer = tokenize(source, 'renamed-' + id);
        assert.ok(resultPointer);
        try {
            const result = JSON.parse(module.UTF8ToString(resultPointer));
            assert.ok(result.tokens.some(token => token.type === type && token.lexeme === lexeme));
        } finally {
            module._free(resultPointer);
        }
    }
    for (const [language, source, expected, lexeme] of [
        ['custom', '\\', 'OPERATOR', '\\'],
        ['python', 'r"foo\\"bar"', 'STRING_LITERAL', 'foo\\"bar'],
        ['c', "'x\ny'", 'ERROR', 'x'],
    ]) {
        const resultPointer = tokenize(source, language);
        assert.ok(resultPointer);
        try {
            const result = JSON.parse(module.UTF8ToString(resultPointer));
            assert.equal(result.tokens[0].type, expected);
            assert.equal(result.tokens[0].lexeme, lexeme);
            if (expected === 'ERROR') assert.match(result.error, /Unterminated character literal/);
        } finally {
            module._free(resultPointer);
        }
    }
    console.log('WebAssembly tokenization and exception handling passed');
}).catch(error => {
    console.error(error);
    process.exitCode = 1;
});
