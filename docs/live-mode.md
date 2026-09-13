# Jaguar Live Reload Mode

Live mode activates automatic source file watching and hot reloading.

Activation via CLI:
```bash
jag -live=1 hello.jag
```

Programmatic activation inside source:
```jaguar
live = "1";
```

When a compile error occurs during live mode, Jaguar displays the diagnostic error and preserves the previously running working process.
