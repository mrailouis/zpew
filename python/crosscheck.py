"""Cross-check zpew's GARCH(1,1) fits against the Python arch library."""

from __future__ import annotations

import argparse
import csv
import json
import math
import subprocess
import sys
import warnings
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from arch import arch_model

REPO_ROOT = Path(__file__).resolve().parent.parent
MANIFEST_PATH = REPO_ROOT / "data" / "manifest.json"
LIKELIHOOD_TOLERANCE = 1e-4
SHAPE_TOLERANCE = 1e-3
OMEGA_RELATIVE_TOLERANCE = 0.01


@dataclass(frozen=True)
class Params:
    omega: float
    alpha: float
    beta: float


def read_returns(snapshot: Path) -> list[float]:
    with snapshot.open(newline="", encoding="ascii") as stream:
        closes = [float(row["close"]) for row in csv.DictReader(stream)]
    return [math.log(closes[i] / closes[i - 1]) for i in range(1, len(closes))]


def log_likelihood(returns: list[float], params: Params, backcast: float) -> float:
    variance = params.omega + (params.alpha + params.beta) * backcast
    total = 0.0
    for r in returns:
        if not variance > 0.0:
            return -math.inf
        total += math.log(variance) + (r * r) / variance
        variance = params.omega + params.alpha * (r * r) + params.beta * variance
    return -0.5 * total


def fit_zpew(cli: Path, symbol: str) -> dict:
    output = subprocess.run(
        [str(cli), "fit", "--symbol", symbol, "--model", "garch", "--manifest", str(MANIFEST_PATH)],
        check=True, capture_output=True, text=True).stdout
    return json.loads(output)


def arch_scale(returns: list[float]) -> float:
    deviation = float(np.std(returns))
    return 10.0 ** -math.floor(math.log10(deviation))


def fit_arch_at(returns: list[float], backcast: float, scale: float) -> Params | None:
    scaled = np.asarray(returns) * scale
    model = arch_model(scaled, mean="Zero", vol="GARCH", p=1, q=1, dist="normal", rescale=False)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        result = model.fit(disp="off", backcast=backcast * scale * scale,
                           options={"maxiter": 2000, "ftol": 1e-12})
    if result.convergence_flag != 0:
        return None
    return Params(omega=float(result.params["omega"]) / (scale * scale),
                  alpha=float(result.params["alpha[1]"]),
                  beta=float(result.params["beta[1]"]))


def fit_arch(returns: list[float], backcast: float) -> tuple[Params, float]:
    preferred = arch_scale(returns)
    for scale in (preferred, preferred * 10.0, preferred / 10.0):
        params = fit_arch_at(returns, backcast, scale)
        if params is not None:
            return params, scale
    raise RuntimeError(f"arch did not converge at scales {preferred / 10:g} to {preferred * 10:g}")


def compare(cli: Path, entry: dict) -> dict:
    returns = read_returns(MANIFEST_PATH.parent / entry["file"])
    zpew = fit_zpew(cli, entry["symbol"])
    backcast = sum(r * r for r in returns) / len(returns)
    if not math.isclose(backcast, zpew["backcast"], rel_tol=1e-12):
        raise RuntimeError(f"backcast differs: python {backcast}, zpew {zpew['backcast']}")
    zpew_params = Params(zpew["omega"], zpew["alpha"], zpew["beta"])
    arch_params, scale = fit_arch(returns, backcast)
    ll_zpew = log_likelihood(returns, zpew_params, backcast)
    ll_arch = log_likelihood(returns, arch_params, backcast)
    shape_ok = (abs(zpew_params.alpha - arch_params.alpha) <= SHAPE_TOLERANCE
                and abs(zpew_params.beta - arch_params.beta) <= SHAPE_TOLERANCE
                and abs(zpew_params.omega - arch_params.omega)
                <= OMEGA_RELATIVE_TOLERANCE * arch_params.omega)
    difference = ll_zpew - ll_arch
    if difference < -LIKELIHOOD_TOLERANCE:
        status = "FAIL"
    elif difference > LIKELIHOOD_TOLERANCE:
        status = "zpew higher"
    elif shape_ok:
        status = "ok"
    else:
        status = "flat likelihood"
    return {"symbol": entry["symbol"], "observations": len(returns), "zpew": zpew_params,
            "arch": arch_params, "ll_difference": difference, "scale": scale, "status": status}


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", required=True, type=Path, help="path to the zpew-cli binary")
    parser.add_argument("--only", default="", help="comma-separated symbols")
    args = parser.parse_args(argv)

    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    wanted = {s.strip().upper() for s in args.only.split(",") if s.strip()}
    entries = [e for e in manifest["assets"] if not wanted or e["symbol"] in wanted]

    print(f"{'symbol':<8} {'obs':>6} {'scale':>6}  {'omega zpew / arch':>25}  "
          f"{'alpha zpew / arch':>17}  {'beta zpew / arch':>17}  {'ll zpew - arch':>14}  status")
    counts = {"ok": 0, "zpew higher": 0, "flat likelihood": 0, "FAIL": 0}
    for entry in entries:
        try:
            row = compare(args.cli, entry)
        except (RuntimeError, subprocess.CalledProcessError) as error:
            counts["FAIL"] += 1
            print(f"{entry['symbol']:<8} FAILED: {error}")
            continue
        counts[row["status"]] += 1
        z, a = row["zpew"], row["arch"]
        print(f"{row['symbol']:<8} {row['observations']:>6} {row['scale']:>6g}  "
              f"{z.omega:.4e} / {a.omega:.4e}  "
              f"{z.alpha:.5f} / {a.alpha:.5f}  {z.beta:.5f} / {a.beta:.5f}  "
              f"{row['ll_difference']:>14.3e}  {row['status']}")

    print(f"\n{counts['ok']} ok, {counts['zpew higher']} zpew higher, "
          f"{counts['flat likelihood']} flat likelihood, {counts['FAIL']} failed, "
          f"of {len(entries)} assets")
    return 1 if counts["FAIL"] else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
