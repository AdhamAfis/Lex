import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEX = ROOT / 'lex'


class LanguageCases(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.source = Path(self.tmp.name) / 'input.txt'
        self.output = Path(self.tmp.name) / 'tokens.json'

    def analyze(self, language, source, *options):
        self.source.write_text(source)
        self.output.unlink(missing_ok=True)
        result = subprocess.run([str(LEX), '-l', language, '-e', 'json', '-o', str(self.output),
                                 *options, str(self.source)], cwd=ROOT, capture_output=True,
                                text=True, timeout=3)
        tokens = json.loads(self.output.read_text())['tokens'] if self.output.exists() else []
        return result, [(token['type'], token['lexeme']) for token in tokens]

    def test_numbers_in_each_language(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            with self.subTest(language=language):
                result, tokens = self.analyze(language, '42 3.14 0x2A')
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('INTEGER', '42'), ('FLOAT', '3.14'), ('HEX', '0x2A'), ('EOF', '')])

    def test_strings_in_each_language(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            with self.subTest(language=language):
                result, tokens = self.analyze(language, '"hello" "a\\\"b"')
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('STRING_LITERAL', 'hello'), ('STRING_LITERAL', 'a\\"b'), ('EOF', '')])

    def test_comments_in_each_language(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            with self.subTest(language=language):
                source = '# note\nvalue' if language == 'python' else '// note\nvalue'
                result, tokens = self.analyze(language, source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('COMMENT', ' note'), ('IDENTIFIER', 'value'), ('EOF', '')])

    def test_operators_in_each_language(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            with self.subTest(language=language):
                result, tokens = self.analyze(language, 'a == b + 2')
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('IDENTIFIER', 'a'), ('COMPARISON_OPERATOR', '=='),
                                          ('IDENTIFIER', 'b'), ('ARITHMETIC_OPERATOR', '+'),
                                          ('INTEGER', '2'), ('EOF', '')])

    def test_unterminated_strings_in_each_language(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            with self.subTest(language=language):
                result, tokens = self.analyze(language, '"unfinished')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unterminated string literal', result.stdout + result.stderr)
                self.assertEqual(tokens, [])

    def test_unterminated_block_comments_in_c_family(self):
        for language in ('c', 'cpp', 'java', 'js'):
            with self.subTest(language=language):
                result, tokens = self.analyze(language, '/* unfinished')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unterminated comment', result.stdout + result.stderr)
                self.assertEqual(tokens, [])

    def test_language_specific_literals(self):
        cases = [
            ('c', '0xFF 077 /*note*/', [('HEX', '0xFF'), ('OCTAL', '077'), ('COMMENT', 'note*/')]),
            ('cpp', "0b101 'x'", [('BINARY', '0b101'), ('CHAR_LITERAL', 'x')]),
            ('java', '0b101L 4.5f', [('BINARY', '0b101L'), ('FLOAT', '4.5f')]),
            ('python', "0o17 0b10 'hello'", [('OCTAL', '0o17'), ('BINARY', '0b10'), ('STRING_LITERAL', 'hello')]),
            ('js', '0o17 0b10 `hello` ===', [('OCTAL', '0o17'), ('BINARY', '0b10'),
                                            ('STRING_LITERAL', 'hello'), ('COMPARISON_OPERATOR', '===')]),
        ]
        for language, source, expected in cases:
            with self.subTest(language=language):
                result, tokens = self.analyze(language, source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, expected + [('EOF', '')])

    def test_python_triple_quoted_string(self):
        result, tokens = self.analyze('python', '"""hello"""')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens, [('STRING_LITERAL', 'hello'), ('EOF', '')])
        result, tokens = self.analyze('python', "r'''hello'''")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens, [('STRING_LITERAL', 'hello'), ('EOF', '')])

    def test_additional_language_literals(self):
        for language, source, expected in [
            ('c', '1.0f 1e3f', [('FLOAT', '1.0f'), ('SCIENTIFIC', '1e3f')]),
            ('python', 'R"hello"', [('STRING_LITERAL', 'hello')]),
            ('js', 'a?.name', [('IDENTIFIER', 'a'), ('OPERATOR', '?.'), ('IDENTIFIER', 'name')]),
        ]:
            with self.subTest(language=language):
                result, tokens = self.analyze(language, source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, expected + [('EOF', '')])

    def test_c_decimal_forms(self):
        result, tokens = self.analyze('c', '.5 1. 1.e2 .5f')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(tokens, [('FLOAT', '.5'), ('FLOAT', '1.'), ('SCIENTIFIC', '1.e2'),
                                  ('FLOAT', '.5f'), ('EOF', '')])

    def test_incomplete_numeric_literals(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            for source in ('0x', '1e+'):
                with self.subTest(language=language, source=source):
                    result, _ = self.analyze(language, source)
                    self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_javascript_optional_chain_before_decimal(self):
        result, tokens = self.analyze('js', 'a?.1:2')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens, [('IDENTIFIER', 'a'), ('OPERATOR', '?'), ('FLOAT', '.1'),
                                  ('OPERATOR', ':'), ('INTEGER', '2'), ('EOF', '')])

    def test_c_character_literal(self):
        result, tokens = self.analyze('c', "'x'")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens, [('CHAR_LITERAL', 'x'), ('EOF', '')])

    def test_java_character_literal(self):
        result, tokens = self.analyze('java', "'x'")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens, [('CHAR_LITERAL', 'x'), ('EOF', '')])

    def test_custom_rule_order_and_keyword_classification(self):
        config = Path(self.tmp.name) / 'custom.json'
        config.write_text(json.dumps({
            'name': 'Example', 'keywords': ['loop'],
            'tokenRules': [
                {'name': 'special', 'pattern': 'item[0-9]+', 'type': 9, 'precedence': 1},
                {'name': 'identifier', 'pattern': '[a-z][a-z0-9]*', 'type': 8, 'precedence': 1},
            ],
        }))
        result, tokens = self.analyze('cpp', 'item12 loop name', '-c', str(config))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(tokens, [('KEYWORD', 'item12'), ('KEYWORD', 'loop'),
                                  ('IDENTIFIER', 'name'), ('EOF', '')])

    def test_cpp_raw_strings(self):
        for source, value in [('R"(a"b)"', 'a"b'), ('R"tag(a"b)tag"', 'a"b')]:
            with self.subTest(source=source):
                result, tokens = self.analyze('cpp', source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('STRING_LITERAL', value), ('EOF', '')])

    def test_single_line_string_rejects_newline(self):
        for language in ('c', 'cpp', 'java', 'python', 'js'):
            with self.subTest(language=language):
                result, _ = self.analyze(language, '"abc\nnext"')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unterminated string', result.stdout + result.stderr)

    def test_multiline_strings_remain_valid(self):
        for language, source in [('python', '"""a\nb"""'), ('js', '`a\nb`')]:
            with self.subTest(language=language):
                result, tokens = self.analyze(language, source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('STRING_LITERAL', 'a\nb'), ('EOF', '')])

    def test_python_raw_single_line_string_rejects_newline(self):
        result, _ = self.analyze('python', 'r"abc\nnext"')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Unterminated string', result.stdout + result.stderr)

    def test_scientific_and_suffix_numbers(self):
        for language, source, expected in [
            ('java', '1e3 1.5e-2f', [('SCIENTIFIC', '1e3'), ('SCIENTIFIC', '1.5e-2f')]),
            ('cpp', '1.0f 2.0L 1e3f', [('FLOAT', '1.0f'), ('FLOAT', '2.0L'), ('SCIENTIFIC', '1e3f')]),
        ]:
            with self.subTest(language=language):
                result, tokens = self.analyze(language, source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, expected + [('EOF', '')])

    def test_config_export_preserves_unicode(self):
        plugin = Path(self.tmp.name) / 'custom_config.json'
        plugin.write_text(json.dumps({'name': 'Custom', 'keywords': ['café'],
                                      'stringConfig': {'stringDelimiters': [{'start': '«', 'end': '»'}]}}))
        exported = Path(self.tmp.name) / 'export.json'
        result = subprocess.run([str(LEX), '-p', self.tmp.name, '--export-config', 'custom', str(exported)],
                                cwd=ROOT, capture_output=True, text=True, timeout=3)
        self.assertEqual(result.returncode, 0, result.stderr)
        config = json.loads(exported.read_text())
        self.assertEqual(config['keywords'], ['café'])
        self.assertEqual(config['stringConfig']['stringDelimiters'], [{'start': '«', 'end': '»'}])

    def test_custom_literal_backslash_regex(self):
        plugins = Path(self.tmp.name) / 'plugins'
        plugins.mkdir()
        config = plugins / 'custom_config.json'
        config.write_text(json.dumps({'name': 'Custom', 'tokenRules': [
            {'name': 'backslash', 'pattern': r'\\', 'type': 10, 'precedence': 1}]}))
        result, tokens = self.analyze('cpp', '\\', '-c', str(config))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(tokens, [('OPERATOR', '\\'), ('EOF', '')])
        exported = Path(self.tmp.name) / 'export.json'
        result = subprocess.run([str(LEX), '-p', str(plugins), '--export-config', 'custom', str(exported)],
                                cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        result, tokens = self.analyze('cpp', '\\', '-c', str(exported))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(tokens, [('OPERATOR', '\\'), ('EOF', '')])

    def test_export_reload_preserves_rules_builtins_and_doc_comments(self):
        for language in ('python', 'java'):
            with self.subTest(language=language):
                exported = Path(self.tmp.name) / (language + '.json')
                result = subprocess.run([str(LEX), '--export-config', language, str(exported)],
                                        cwd=ROOT, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0)
                original = json.loads((ROOT / 'plugins' / (language + '_config.json')).read_text())
                loaded = json.loads(exported.read_text())
                self.assertEqual(set(loaded.get('builtins', [])), set(original.get('builtins', [])))
                for field in ('docCommentStarts', 'docCommentDelimiters'):
                    self.assertEqual(loaded['commentConfig'].get(field, []), original['commentConfig'].get(field, []))
                self.assertEqual(loaded['stringConfig'].get('formattedStringPrefixes', []),
                                 original['stringConfig'].get('formattedStringPrefixes', []))
                self.assertEqual(loaded['additionalOperators'], original['additionalOperators'])
                self.assertEqual(loaded['tokenRules'], original['tokenRules'])

    def test_python_raw_string_escaped_quote(self):
        result, tokens = self.analyze('python', 'r"foo\\"bar"')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(tokens, [('STRING_LITERAL', 'foo\\"bar'), ('EOF', '')])

    def test_character_literal_rejects_newline(self):
        for language in ('c', 'cpp', 'java'):
            with self.subTest(language=language):
                result, _ = self.analyze(language, "'x\ny'")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unterminated character literal', result.stdout + result.stderr)

    def test_python_assignment_expression_and_colon(self):
        result, tokens = self.analyze('python', 'x := 1:')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens, [('IDENTIFIER', 'x'), ('ASSIGNMENT_OPERATOR', ':='),
                                  ('INTEGER', '1'), ('DELIMITER', ':'), ('EOF', '')])

    def test_renamed_python_configuration_keeps_assignment_expression(self):
        config = json.loads((ROOT / 'plugins/python_config.json').read_text())
        config['name'] = 'MyPython'
        custom = Path(self.tmp.name) / 'custom.json'
        custom.write_text(json.dumps(config))
        result, tokens = self.analyze('cpp', 'x := 1', '-c', str(custom))
        self.assertEqual(result.returncode, 0)
        self.assertEqual(tokens[1], ('ASSIGNMENT_OPERATOR', ':='))

    def test_renamed_configurations_keep_language_behavior(self):
        for language, source, expected in [
            ('cpp', 'R"tag(a"b)tag"', ('STRING_LITERAL', 'a"b')),
            ('js', 'a===b', ('COMPARISON_OPERATOR', '===')),
            ('python', '10 // 2', ('OPERATOR', '//')),
        ]:
            with self.subTest(language=language):
                config = json.loads((ROOT / 'plugins' / (language + '_config.json')).read_text())
                config['name'] = 'Renamed'
                custom = Path(self.tmp.name) / 'renamed.json'
                custom.write_text(json.dumps(config))
                result, tokens = self.analyze('cpp', source, '-c', str(custom))
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(expected, tokens)

    def test_exported_cpp_configuration_keeps_raw_strings(self):
        exported = Path(self.tmp.name) / 'cpp.json'
        result = subprocess.run([str(LEX), '--export-config', 'cpp', str(exported)],
                                cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0)
        config = json.loads(exported.read_text())
        config['name'] = 'Renamed'
        exported.write_text(json.dumps(config))
        result, tokens = self.analyze('cpp', 'R"tag(a"b)tag"', '-c', str(exported))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(tokens, [('STRING_LITERAL', 'a"b'), ('EOF', '')])

    def test_custom_escape_character_for_strings_and_characters(self):
        custom = Path(self.tmp.name) / 'custom.json'
        custom.write_text(json.dumps({'name': 'Custom', 'stringConfig': {'escapeChar': '%',
            'stringDelimiters': [{'start': '"', 'end': '"'}],
            'charDelimiters': [{'start': "'", 'end': "'"}]}}))
        result, tokens = self.analyze('cpp', '"a%"b" \'%\'\'', '-c', str(custom))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(tokens, [('STRING_LITERAL', 'a%"b'), ('CHAR_LITERAL', "%'") , ('EOF', '')])

    def test_java_does_not_accept_preprocessor_directives(self):
        result, _ = self.analyze('java', '#include hi')
        self.assertNotEqual(result.returncode, 0)

    def test_c_cpp_preprocessor_still_works(self):
        for language in ('c', 'cpp'):
            with self.subTest(language=language):
                result, tokens = self.analyze(language, '#include <stdio.h>')
                self.assertEqual(result.returncode, 0)
                self.assertEqual(tokens[0][0], 'PREPROCESSOR')

    def test_python_formatted_strings(self):
        for source in ['f"hello {name}"', 'F\'hello {name}\'', 'fr"hello {name}"', 'rf"hello {name}"']:
            with self.subTest(source=source):
                result, tokens = self.analyze('python', source)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(tokens, [('STRING_LITERAL', 'hello {name}'), ('EOF', '')])

    def test_documentation_comment_has_attribute(self):
        result, _ = self.analyze('java', '/** docs */\n/* normal */\n/**/')
        self.assertEqual(result.returncode, 0)
        tokens = json.loads(self.output.read_text())['tokens']
        self.assertEqual(tokens[0].get('attributes'), '[documentation]')
        self.assertEqual(tokens[1].get('attributes'), '[multi-line]')
        self.assertEqual(tokens[2].get('attributes'), '[multi-line]')


if __name__ == '__main__':
    unittest.main()
