# Input and Video probes

Run headlessly with:

```sh
python3 tests/run.py --suite runtime --report reports/RUNTIME.md
```

The suite checks local wrapper behavior and safe state/no-op contracts. Passing means the local checks passed; it does not establish retail parity. Device input, text entry, texture assets, draw output, and full game scenes need platform fixtures or a working retail capture.
