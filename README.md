# Zingers

## Getting started

To install dependencies run

```
sudo ./init.sh
```

To tell clangd about these dependencies (i.e. to get LSP support) add the
following to your clangd configuration (located at
`~/Library/Preferences/clangd/config.yaml`)

```yaml
CompileFlags:
  Add:
    [
      -I/usr/local/include/uWebSockets,
      -I/usr/local/include/uSockets,
      -I/usr/local/include/glaze,
    ]
```

I'd also recommend referencing
[my clangd config](https://github.com/ConnerRose/dotfiles/blob/main/Library/Preferences/clangd/config.yaml)
which sets a lot of other options I find useful, and that I will be using while
leading the development of this project.
