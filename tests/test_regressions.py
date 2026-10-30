import json
import subprocess
import tempfile
import unittest
import os
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / 'lex'


class LexRegressions(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'sample.cpp'

    def run_lex(self, source, *args, cwd=ROOT):
        self.path.write_text(source)
        return subprocess.run([str(BINARY), '-v', *args, str(self.path)], cwd=cwd,
                              capture_output=True, text=True, timeout=2)

    def test_incomplete_literals_and_comments_terminate_with_errors(self):
        for source in ['"', '"abc\\', '/*', "'", "'\\"]:
            with self.subTest(source=source):
                result = self.run_lex(source)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unterminated', result.stdout + result.stderr)

    def test_directory_is_not_a_source_file(self):
        result = subprocess.run([str(BINARY), '-v', self.tmp.name], cwd=ROOT, capture_output=True)
        self.assertNotEqual(result.returncode, 0)

    def test_invalid_plugin_token_types_are_rejected(self):
        config = Path(self.tmp.name) / 'config.json'
        for token_type in (-1, 25, 26, 27, 999):
            config.write_text(json.dumps({'tokenRules': [{'name': 'bad', 'pattern': 'x', 'type': token_type, 'precedence': 1}]}))
            result = self.run_lex('x', '-c', str(config))
            self.assertNotEqual(result.returncode, 0, str(token_type))

    @unittest.skipUnless(os.name == 'posix', 'requires POSIX resource limits')
    def test_failed_exports_preserve_existing_file(self):
        import resource
        import signal
        output = Path(self.tmp.name) / 'out'
        self.path.write_text('int x;')
        def limit():
            signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
            resource.setrlimit(resource.RLIMIT_FSIZE, (0, 0))
        for args in (['-e', 'json', '-o', str(output), str(self.path)],
                     ['--export-config', 'cpp', str(output)]):
            output.write_text('original')
            result = subprocess.run([str(BINARY), *args], cwd=ROOT, capture_output=True, preexec_fn=limit)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_text(), 'original')

    def test_non_plugin_json_does_not_shadow_named_plugin(self):
        plugins = Path(self.tmp.name) / 'plugins'
        plugins.mkdir()
        (plugins / 'custom_config.json').write_text(json.dumps({'name': 'Custom', 'keywords': ['foo']}))
        (plugins / 'tokens.json').write_text(json.dumps({'tokens': []}))
        result = self.run_lex('foo', '-p', str(plugins), '-l', 'custom')
        self.assertIn("KEYWORD: 'foo'", result.stdout)

    def test_duplicate_plugin_names_and_aliases_are_rejected(self):
        plugins = Path(self.tmp.name) / 'plugins'
        plugins.mkdir()
        (plugins / 'cpp_config.json').write_text(json.dumps({'name': 'C++'}))
        (plugins / 'other.json').write_text(json.dumps({'name': 'cpp'}))
        result = self.run_lex('foo', '-p', str(plugins), '-l', 'cpp')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Duplicate language plugin', result.stderr)

    def test_duplicate_symbols_remain_valid(self):
        source = Path(self.tmp.name) / 'symbols.cpp'
        executable = Path(self.tmp.name) / 'symbols'
        source.write_text(r'''#include "SymbolTable.h"
#include <cassert>
int main() {
    SymbolTable table;
    auto *first = table.addSymbol("x", SymbolKind::VARIABLE, "int", 1, 1, true);
    auto *same = table.addSymbol("x", SymbolKind::VARIABLE, "int", 2, 1, true);
    assert(first == same);
    assert(table.findSymbols("x").size() == 1);
    auto *scope = table.createScope("inner", ScopeType::BLOCK, 1, 1, "", nullptr);
    table.enterScope(scope);
    auto *other = table.addSymbol("x", SymbolKind::VARIABLE, "int", 3, 1, true);
    assert(other != first);
    assert(table.findSymbols("x").size() == 2);
    assert(first->getName() == "x");
}''')
        subprocess.run(['c++', '-std=c++17', '-I', str(ROOT / 'src'), str(source),
                        str(ROOT / 'src/SymbolTable.cpp'), '-o', str(executable)], check=True, capture_output=True)
        subprocess.run([str(executable)], check=True)

    def test_export_cannot_replace_custom_config(self):
        config = Path(self.tmp.name) / 'config.json'
        original = json.dumps({'name': 'Custom', 'keywords': ['foo']})
        for alias in ('same', 'symlink', 'hardlink'):
            config.write_text(original)
            output = Path(self.tmp.name) / 'output'
            if alias == 'same': output = config
            elif alias == 'symlink': output.symlink_to(config)
            else: os.link(config, output)
            result = self.run_lex('foo', '-c', str(config), '-e', 'json', '-o', str(output))
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(config.read_text(), original)
            if alias != 'same': output.unlink()

    def test_custom_config_works_without_plugins(self):
        directory = Path(self.tmp.name) / 'empty'
        directory.mkdir()
        config = Path(self.tmp.name) / 'config.json'
        config.write_text(json.dumps({'name': 'Custom', 'keywords': ['foo']}))
        result = self.run_lex('foo', '-p', str(directory), '-c', str(config))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("KEYWORD: 'foo'", result.stdout)

    def test_config_export_error_does_not_abort(self):
        import shutil
        directory = Path(self.tmp.name) / 'plugins'
        directory.mkdir()
        shutil.copy(ROOT / 'plugins/c_config.json', directory)
        result = subprocess.run([str(BINARY), '-p', str(directory), '--export-config', 'missing',
                                 str(Path(self.tmp.name) / 'out')], cwd=ROOT, capture_output=True)
        self.assertGreater(result.returncode, 0)
        self.assertNotIn(b'terminating due to uncaught', result.stderr)

    def test_xml_rejects_forbidden_control_characters(self):
        output = Path(self.tmp.name) / 'tokens.xml'
        output.write_text('previous')
        result = self.run_lex('// bad\x01', '-e', 'xml', '-o', str(output))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output.read_text(), 'previous')

    def test_exported_config_does_not_claim_unused_number_patterns(self):
        output = Path(self.tmp.name) / 'config.json'
        result = subprocess.run([str(BINARY), '--export-config', 'c', str(output)], cwd=ROOT, capture_output=True)
        self.assertEqual(result.returncode, 0)
        self.assertNotIn('numberConfig', json.loads(output.read_text()))

    def test_keywords_and_symbols(self):
        result = self.run_lex('int main() { return main; }')
        self.assertIn("KEYWORD: 'int'", result.stdout)
        self.assertIn("KEYWORD: 'return'", result.stdout)
        self.assertIn('1 unique symbols', result.stdout)

    def test_javascript_types(self):
        result = self.run_lex('const $x = 3.14; $x += 1;', '-l', 'js')
        for expected in ["KEYWORD: 'const'", "IDENTIFIER: '$x'", "ASSIGNMENT_OPERATOR: '='", "ASSIGNMENT_OPERATOR: '+='", "FLOAT: '3.14'", "SEMICOLON: ';'"]:
            self.assertIn(expected, result.stdout)

    def test_other_bundled_language_keywords(self):
        for language, keyword in [('c', 'return'), ('java', 'class'), ('python', 'def')]:
            with self.subTest(language=language):
                result = self.run_lex(keyword + ' sample', '-l', language)
                self.assertIn("KEYWORD: '" + keyword + "'", result.stdout)
                self.assertIn('1 unique symbols', result.stdout)

    def test_directives_end_at_newline_and_allow_indentation(self):
        result = self.run_lex('  #include <x>\n#endif\nint x;\n')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("PREPROCESSOR: 'include <x>'", result.stdout)
        self.assertIn("PREPROCESSOR: 'endif", result.stdout)
        self.assertIn("KEYWORD: 'int'", result.stdout)

    def test_json_export_escapes_values_and_attributes(self):
        output = Path(self.tmp.name) / 'tokens.json'
        result = self.run_lex('int x = "a\\\"b";', '-e', 'json', '-o', str(output))
        self.assertEqual(result.returncode, 0, result.stderr)
        data = json.loads(output.read_text())
        self.assertTrue(any(t['lexeme'] == 'a\\"b' for t in data['tokens']))
        self.assertTrue(any('attributes' in t for t in data['tokens']))

    def test_literal_and_comment_locations(self):
        result = self.run_lex("'x' \"abc\" /* c */")
        self.assertIn("CHAR_LITERAL: 'x' at " + str(self.path) + ':1:1', result.stdout)
        self.assertIn("STRING_LITERAL: 'abc' at " + str(self.path) + ':1:5', result.stdout)
        self.assertIn(' at ' + str(self.path) + ':1:11', result.stdout)

    def test_failures_and_empty_file_exit_status(self):
        self.assertNotEqual(self.run_lex('@').returncode, 0)
        result = subprocess.run([str(BINARY), str(self.path.with_name('missing'))], cwd=ROOT, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        empty = self.run_lex('')
        self.assertEqual(empty.returncode, 0)
        self.assertIn("EOF: ''", empty.stdout)
        self.assertNotEqual(self.run_lex('int x;', '-e', 'bad', '-o', str(self.path) + '.out').returncode, 0)
        self.assertNotEqual(self.run_lex('int x;', '-e', 'json', '-o', self.tmp.name).returncode, 0)
        self.assertNotEqual(self.run_lex('int x;', '-e', 'json').returncode, 0)

    def test_custom_plugin_directory_without_default(self):
        result = self.run_lex('int x;', '-p', str(ROOT / 'plugins'), cwd=self.tmp.name)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("KEYWORD: 'int'", result.stdout)

    def test_interactive_exits_when_input_closes(self):
        result = subprocess.run([str(BINARY), '-i'], cwd=ROOT, input='',
                                capture_output=True, text=True, timeout=2)
        self.assertEqual(result.returncode, 0)

    def test_export_rejects_source_and_aliases(self):
        source = 'int x;'
        for alias in ['same', 'symlink', 'hardlink']:
            with self.subTest(alias=alias):
                self.path.write_text(source)
                output = Path(self.tmp.name) / 'out'
                if alias == 'same': output = self.path
                elif alias == 'symlink': output.symlink_to(self.path)
                else: os.link(self.path, output)
                result = subprocess.run([str(BINARY), '-e', 'json', '-o', str(output), str(self.path)],
                                        cwd=ROOT, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(self.path.read_text(), source)
                if alias != 'same': output.unlink()

    def test_operators_are_longest_valid_tokens(self):
        for language, source, operators in [
            ('cpp', 'a==b && c<=d;', ["COMPARISON_OPERATOR: '=='", "LOGICAL_OPERATOR: '&&'", "COMPARISON_OPERATOR: '<='"]),
            ('cpp', 'a===b;', ["COMPARISON_OPERATOR: '=='", "ASSIGNMENT_OPERATOR: '='"]),
            ('c', 'a++ + b;', ["ARITHMETIC_OPERATOR: '++'", "ARITHMETIC_OPERATOR: '+'"]),
            ('js', 'a+-b;', ["ARITHMETIC_OPERATOR: '+'", "ARITHMETIC_OPERATOR: '-'"])
        ]:
            with self.subTest(language=language):
                result = self.run_lex(source, '-l', language)
                for operator in operators: self.assertIn(operator, result.stdout)
                self.assertNotIn("OPERATOR: '+-'", result.stdout)

    def test_cpp_binary_literal(self):
        result = self.run_lex('int x=0b101;', '-l', 'cpp')
        self.assertIn("BINARY: '0b101'", result.stdout)
        self.assertNotIn("IDENTIFIER: 'b101'", result.stdout)

    def test_custom_operator_rule_preserves_its_match(self):
        config = Path(self.tmp.name) / 'custom.json'
        config.write_text(json.dumps({'name': 'Custom', 'characterSets': {'operators': '@'},
                                     'tokenRules': [{'name': 'custom', 'pattern': '@@', 'type': 10, 'precedence': 1}]}))
        result = self.run_lex('@@', '-c', str(config))
        self.assertIn("OPERATOR: '@@'", result.stdout)

    def test_empty_comment_and_string_delimiters_rejected(self):
        for config in [
            {'commentConfig': {'multiLineCommentDelimiters': [{'start': '', 'end': ''}]}},
            {'commentConfig': {'singleLineCommentStarts': ['']}},
            {'stringConfig': {'stringDelimiters': [{'start': '"', 'end': ''}]}},
            {'stringConfig': {'charDelimiters': [{'start': '', 'end': "'"}]}}
        ]:
            with self.subTest(config=config):
                file = Path(self.tmp.name) / 'config.json'
                file.write_text(json.dumps(config))
                result = self.run_lex('x', '-c', str(file))
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('delimiter', (result.stdout + result.stderr).lower())

    def test_embedded_zero_byte_is_rejected(self):
        result = self.run_lex('int a;\0int b;')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('null', (result.stderr + result.stdout).lower())

    def test_explicit_cpp_overrides_filename(self):
        self.path = self.path.with_suffix('.js')
        result = self.run_lex('int x;', '-l', 'c++')
        self.assertIn('(Language: C++)', result.stdout)
        self.assertIn("KEYWORD: 'int'", result.stdout)

    def test_xml_escapes_filename(self):
        self.path = self.path.with_name('a&b.cpp')
        output = Path(self.tmp.name) / 'tokens.xml'
        result = self.run_lex('int x;', '-e', 'xml', '-o', str(output))
        self.assertEqual(result.returncode, 0)
        self.assertEqual(ET.parse(output).findtext('./token/location/filename'), str(self.path))

    def test_header_changes_rebuild_dependents(self):
        result = subprocess.run(['make', '-n', '-W', 'src/Token.h'], cwd=ROOT,
                                capture_output=True, text=True, check=True)
        self.assertIn('src/Token.cpp', result.stdout)

    def test_failed_analysis_preserves_previous_export(self):
        output = Path(self.tmp.name) / 'tokens.json'
        output.write_text('previous results')
        result = self.run_lex('@', '-e', 'json', '-o', str(output))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output.read_text(), 'previous results')
        self.assertNotIn('Tokens exported', result.stdout)

    def test_wasm_exception_returns_valid_json(self):
        tmp = Path(self.tmp.name)
        (tmp / 'emscripten').mkdir()
        (tmp / 'emscripten/emscripten.h').write_text('#define EMSCRIPTEN_KEEPALIVE\n')
        (tmp / 'check.cpp').write_text(
            '#include <cstdio>\n#include <cstdlib>\n'
            'extern "C" char* tokenizeString(const char*, const char*);\n'
            'int main() { char* p = tokenizeString("x", "missing"); '
            'if (!p) return 1; puts(p); free(p); }\n')
        objects = ['Lexer', 'Token', 'LanguageConfig', 'SymbolTable', 'ExportFormatter', 'ConfigLoader', 'LanguagePlugin']
        command = [os.environ.get('CXX', 'c++'), '-std=c++17', '-I' + str(tmp), str(tmp / 'check.cpp'),
                   str(ROOT / 'src/wasm_bindings.cpp')] + [str(ROOT / 'src' / (name + '.o')) for name in objects]
        subprocess.run(command + ['-o', str(tmp / 'check')], check=True)
        result = subprocess.run([str(tmp / 'check')], cwd=tmp, capture_output=True, text=True, check=True)
        payload = json.loads(result.stdout.splitlines()[-1])
        self.assertIn('error', payload)
        self.assertEqual(payload['tokens'], [])


if __name__ == '__main__':
    unittest.main()
