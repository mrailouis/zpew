# zpew

zpew is a volatility analytics terminal written in C++20. It loads daily price
history, fits volatility models (rolling realised volatility, EWMA and
GARCH(1,1)), and tests their forecasts out of sample against what actually
happened. It forecasts volatility, not prices, and it does not trade.

The project is under active development. The data layer is in place; the
models, the evaluation engine and the charts are still being built, and no
results have been published yet.

### Building

zpew needs CMake 3.25 or newer and a C++20 compiler. It is developed on macOS
with Apple Clang and on Windows with Visual Studio 2022 or newer. Linux support
is planned. CMake downloads every other dependency (Catch2, nlohmann/json,
Dear ImGui, ImPlot and GLFW) at configure time, pinned by version and hash.

To build on macOS:

```
$ git clone https://github.com/mrailouis/zpew
$ cd zpew
$ cmake --preset macos-release
$ cmake --build --preset macos-release
$ ./build/macos-release/cli/zpew-cli --version
zpew-cli 0.1.0
```

The desktop app is built as `build/macos-release/app/zpew-app.app`.

To build on Windows, run these from a Developer PowerShell for Visual Studio:

```
> cmake --preset windows-msvc
> cmake --build --preset windows-msvc-release
> .\build\windows-msvc\cli\Release\zpew-cli.exe --version
```

`CMakePresets.json` also has debug presets and AddressSanitizer presets
(`macos-asan` and `windows-msvc-asan`).

### Running tests

```
$ cmake --preset macos-debug
$ cmake --build --preset macos-debug
$ ctest --preset macos-debug
$ python3 -m unittest discover -s python
```

On Windows, use the `windows-msvc-debug` build and test presets instead.

### Getting market data

zpew runs on fixed local snapshots of daily prices from
[Tiingo](https://www.tiingo.com). The snapshots are not in this repository,
because Tiingo's free plan only covers personal use. You fetch your own copy
with a free Tiingo account. The API key is only used for fetching; zpew itself
reads the local files and never contacts Tiingo.

1. Create a free account at [tiingo.com](https://www.tiingo.com) and copy your
   API token from the [token page](https://www.tiingo.com/account/api/token).

2. Put the token in a `.env` file in the repository root. Git ignores this file.

   ```
   $ cp .env.example .env
   ```

   Then edit `.env` so it reads `TIINGO_API_KEY=your-token`. Setting
   `TIINGO_API_KEY` in your environment works too and takes precedence.

3. Fetch the assets listed in `data/universe.csv`. The script needs Python 3.9
   or newer and nothing outside the standard library.

   ```
   $ python3 python/fetch_data.py --end 2026-09-30
   ```

   `--end` fixes the last date, so the same command always asks for the same
   range. The free plan allows 50 requests per hour, so a full fetch takes
   about two and a half hours. The script waits for the rate limit on its own
   and can be stopped and restarted; snapshots that are already complete are
   skipped. Use `--only SPY,QQQ` to fetch a subset and `--refresh` to fetch
   again.

4. Check that every snapshot loads and matches the manifest:

   ```
   $ ./build/macos-release/cli/zpew-cli check-data
   ```

`data/manifest.json` records the source, date range, row count and SHA-256 hash
of every snapshot. Tiingo revises adjusted prices after new dividends and
splits, so a later fetch can differ from the committed manifest. In that case
`check-data` reports a hash mismatch, and results computed from your snapshots
will not exactly match published ones.

### Running the evaluation

One command runs the full out-of-sample evaluation from the snapshots:

```
$ ./build/macos-release/cli/zpew-cli eval --config configs/default.toml
```

It writes forecasts, losses, improvements over the baseline and a `run.json`
with the git commit, compiler, config hash and the hash of every snapshot to
`results/default/eval/`. The VaR backtest works the same way and writes to
`results/default/backtest/`:

```
$ ./build/macos-release/cli/zpew-cli backtest --config configs/default.toml
```

 `configs/default.toml` documents every setting. The
command warns when the binary was built from uncommitted changes, because such
results cannot be traced back to a commit.

### Cross-checks

Two scripts compare zpew against independent implementations on every snapshot
in the manifest. The EWMA check only needs the standard library:

```
$ python3 python/crosscheck_ewma.py --cli build/macos-release/cli/zpew-cli
```

The GARCH(1,1) check compares zpew's fits with the
[arch](https://github.com/bashtage/arch) library. Install its pinned
dependencies in a virtual environment first:

```
$ python3 -m venv .venv
$ .venv/bin/pip install -r python/requirements-crosscheck.txt
$ .venv/bin/python python/crosscheck.py --cli build/macos-release/cli/zpew-cli
```

### License

zpew is licensed under the [Apache License 2.0](LICENSE).
