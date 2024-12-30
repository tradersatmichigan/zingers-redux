## About

Zinger's is an ETF market making game created and developed in-house at
Traders at Michigan. Trade sandwich ingredients, make reubens, and profit.

See https://github.com/tradersatmichigan/zingers for last years version.

We're rebuilding for this years UMich Trading Competition, using C++ on the
backend this time around. We're using the fantastic
[µWebSockets](https://github.com/uNetworking/uWebSockets) project to power
the project.

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

> [!NOTE]
> You may have to run `sudo chmod 777 ./uWebSockets/uSockets` in order for make
> instructions to run correctly.

## Backstory

Last year, we hosted the University of Michigan's first trading competition,
the [UMich Quant Convention](https://umquantconvention.com/). During which,
we played Zinger's, but due to technical constraints, we were forced to play
three smaller games of Zinger's simultaneously, instead of one large game, as
we initially planned.

We're rebuilding the game with the intention of improving performance and
reliability.
