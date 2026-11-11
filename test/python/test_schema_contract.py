"""Validate published JSON Schema contracts with an entirely local registry."""

import copy
import json
from pathlib import Path
import runpy
import unittest

from jsonschema import Draft202012Validator
from jsonschema.validators import validator_for
from referencing import Registry, Resource


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
SCHEMAS = {path.name: json.loads(path.read_text(encoding="utf-8")) for path in TOOLS.glob("*.schema.json")}


def make_validator(name, *, custom_recognition=False):
    # Registry has no retrieval callback: unresolved references fail instead of using the network.
    registry = Registry()
    for filename, contents in SCHEMAS.items():
        registry = registry.with_resource((TOOLS / filename).as_uri(), Resource.from_contents(contents))
    if custom_recognition:
        contents = json.loads((ROOT / "test/schema/custom.recognition.schema.json").read_text(encoding="utf-8"))
        registry = registry.with_resource(
            (TOOLS / "custom.recognition.schema.json").as_uri(),
            Resource.from_contents(contents),
        )
    schema = dict(SCHEMAS[name], **{"$id": (TOOLS / name).as_uri()})
    return validator_for(schema)(schema, registry=registry)


class SchemaValidityTest(unittest.TestCase):
    def test_all_published_schemas_are_valid(self):
        self.assertTrue(SCHEMAS, "no published schemas found")
        for name, schema in SCHEMAS.items():
            with self.subTest(schema=name):
                validator_for(schema).check_schema(schema)

    def test_pipeline_uses_draft_2020_12(self):
        self.assertIsInstance(make_validator("pipeline.schema.json"), Draft202012Validator)


class PipelineSchemaTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.validator = make_validator("pipeline.schema.json", custom_recognition=True)

    def test_correct_fixture_as_a_complete_pipeline(self):
        fixture = json.loads((ROOT / "test/schema/pipeline.schema.correct.json").read_text(encoding="utf-8"))
        self.validator.validate(fixture)


def pipeline_fixture_test(name, node, *, valid):
    def test(self):
        errors = list(self.validator.iter_errors({name: node}))
        if valid:
            self.assertFalse(errors, "\n".join(error.message for error in errors))
        else:
            self.assertTrue(errors, f"invalid fixture node {name!r} was accepted")

    return test


for fixture_name, valid in (("correct", True), ("error", False)):
    fixture = json.loads((ROOT / f"test/schema/pipeline.schema.{fixture_name}.json").read_text(encoding="utf-8"))
    for name, node in fixture.items():
        if not name.startswith("$"):
            setattr(
                PipelineSchemaTest,
                f"test_{fixture_name}_{name}",
                pipeline_fixture_test(name, node, valid=valid),
            )


def interface_base():
    return {
        "interface_version": 2,
        "name": "SchemaContract",
        "controller": [{"name": "ADB", "type": "Adb"}],
        "resource": [{"name": "Default", "path": ["./resource"]}],
    }


def import_fragment():
    return {
        "task": [{"name": "Run", "entry": "Entry", "group": ["Main"], "option": ["Path"]}],
        "option": {
            "Path": {
                "type": "input",
                "inputs": [{"name": "path", "input_type": "file", "pipeline_type": "string"}],
                "pipeline_override": {"Entry": {"input_text": "{path}"}},
            }
        },
        "pretask": {"exec": "python", "args": ["prepare.py"], "option": ["Path"]},
        "global_option": ["Path"],
        "setting": [{"name": "General", "option": ["Path"]}],
        "preset": [{"name": "Default", "task": [{"name": "Run", "enabled": True}]}],
        "group": [{"name": "Main", "default_expand": False}],
    }


class InterfaceSchemaTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.interface = make_validator("interface.schema.json")
        cls.imported = make_validator("interface_import.schema.json")
        cls.config = make_validator("interface_config.schema.json")

    def test_minimal_and_complete_interface(self):
        base = interface_base()
        self.interface.validate(base)
        base.update(import_fragment())
        base.update(
            agent=[{"child_exec": "python", "child_args": ["agent.py"], "identifier": "agent"}],
            **{"import": ["./tasks.json"]},
        )
        self.interface.validate(base)

    def test_each_required_interface_field(self):
        for field in ("interface_version", "name", "controller", "resource"):
            value = interface_base()
            del value[field]
            with self.subTest(field=field):
                self.assertFalse(self.interface.is_valid(value))

    def test_invalid_interface_fields(self):
        for field, value in (
            ("interface_version", 1),
            ("name", 123),
            ("controller", [{"name": "ADB", "type": "Unknown"}]),
            ("controller", [{"name": "ADB"}]),
            ("resource", [{"name": "Default"}]),
            ("resource", [{"name": "Default", "path": 123}]),
            ("agent", {"child_exec": "python", "timeout": 30}),
            ("agent", {"child_args": ["agent.py"]}),
            ("import", "tasks.json"),
            ("import", [123]),
            ("unknown", True),
        ):
            document = interface_base()
            document[field] = value
            with self.subTest(field=field, value=value):
                self.assertFalse(self.interface.is_valid(document))

    def test_importable_fields_match_protocol_declaration(self):
        checker = runpy.run_path(str(TOOLS / "check_interface_schema.py"))
        declared = checker["declared_importable_fields"](SCHEMAS["interface.schema.json"])
        accepted = set(SCHEMAS["interface_import.schema.json"]["properties"])
        self.assertEqual(accepted, declared)
        self.assertEqual(accepted, set(import_fragment()))

    def test_imported_fields_validate_individually_and_together(self):
        fragment = import_fragment()
        self.imported.validate(fragment)
        for field, value in fragment.items():
            with self.subTest(field=field):
                self.imported.validate({field: value})
                document = interface_base()
                document[field] = value
                self.interface.validate(document)

    def test_invalid_imported_fields(self):
        for fragment in (
            {"task": [{"name": "Run"}]},
            {"task": [{"name": "Run", "entry": "Entry", "group": "Main"}]},
            {"option": {"Path": {"type": "input"}}},
            {"option": {"Path": {"type": "unknown"}}},
            {"pretask": {"args": []}},
            {"global_option": [123]},
            {"setting": [{"option": ["Path"]}]},
            {"preset": [{"task": []}]},
            {"group": [{"label": "Main"}]},
            {"unknown": {}},
        ):
            with self.subTest(fragment=fragment):
                self.assertFalse(self.imported.is_valid(fragment))
                self.assertFalse(self.interface.is_valid(dict(interface_base(), **fragment)))

    def test_password_input_and_input_types(self):
        for input_type in ("text", "file", "directory"):
            for password in (False, True):
                document = import_fragment()
                field = document["option"]["Path"]["inputs"][0]
                field.update(input_type=input_type, password=password)
                with self.subTest(input_type=input_type, password=password):
                    self.imported.validate(document)
                    self.interface.validate(dict(interface_base(), **document))
                    field["default"] = "value"
                    self.assertEqual(self.imported.is_valid(document), not password)
                    self.assertEqual(self.interface.is_valid(dict(interface_base(), **document)), not password)

    def test_invalid_input_fields(self):
        for update in ({"input_type": "number"}, {"pipeline_type": "float"}, {"password": "true"}):
            document = import_fragment()
            document["option"]["Path"]["inputs"][0].update(update)
            with self.subTest(update=update):
                self.assertFalse(self.imported.is_valid(document))
                self.assertFalse(self.interface.is_valid(dict(interface_base(), **document)))

    def test_saved_configuration(self):
        document = {
            "controller": {"name": "ADB", "type": "Adb"},
            "resource": "Default",
            "task": [{"name": "Run", "option": [{"name": "Path", "value": "example.txt"}]}],
            "adb": {"adb_path": "adb", "address": "127.0.0.1:5555"},
        }
        for welcome in (None, "Announcement", ["Announcement"]):
            with self.subTest(welcome=welcome):
                self.config.validate(dict(document, last_welcome=welcome, last_resolved_welcome=welcome))
        for field in ("controller", "resource", "task"):
            missing = copy.deepcopy(document)
            del missing[field]
            with self.subTest(missing=field):
                self.assertFalse(self.config.is_valid(missing))
        for field, value in (
            ("controller", {"type": "Unknown"}),
            ("resource", 123),
            ("task", [{"option": []}]),
            ("task", [{"name": "Run", "option": [{"name": "Path"}]}]),
            ("last_welcome", []),
            ("last_resolved_welcome", 123),
        ):
            with self.subTest(field=field, value=value):
                self.assertFalse(self.config.is_valid(dict(document, **{field: value})))


if __name__ == "__main__":
    unittest.main()
