# OpenEgg script API

This is an early, source-backed reference for the GameMonkeyScript API. Each registered function and method has its own page. Known retail methods missing from OpenEgg are listed separately.

Build the site from the repository root with `./tools/build_api_docs.sh`. The HTML output is `build/docs/api/html/index.html`.

## Editing the reference

Edit `function_notes.tsv`; do not edit generated pages. Keep one row per registered API entry, sorted by the full `Owner.method` name. The columns are:

- `name`, `description`, `return_type`, and `returns_note`
- `parameters_json`: ordered parameter records with `index`, `name`, `type`, `description`, plus `default` and `optional` when applicable
- `parameters_note`: overload details or evidence that does not fit a single parameter record
- `signature`: the displayed script signature

Use `-` for an empty optional field. Describe what the current binding does, then state separately what retail evidence confirms and what remains unknown. Do not infer retail parity from a successful local build. Retail type counts are in `retail_types.tsv`; recovered methods missing from OpenEgg are in `retail_missing.tsv`. Registered type pages are generated from the bindings.

OpenEgg-specific values with notable field or proxy behavior are listed in `custom_types.tsv`; document their fields in a matching Doxygen page under `docs/api/`. Registered methods also get individual generated pages.
