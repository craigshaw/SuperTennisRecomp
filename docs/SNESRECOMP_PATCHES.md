# snesrecomp integration

This is the single source of truth for the temporary snesrecomp patch series
required by Super Tennis.

## Pinned revisions

The repository pins the submodule to the public integration commit:

```text
3678d0a6d7036217f26e32f6b9087d2933783690
```

That commit is published on
[`craigshaw/snesrecomp`](https://github.com/craigshaw/snesrecomp/tree/codex/super-tennis-runtime)
and is based on upstream commit:

```text
a64932f1af958f7e71a728ac1235d6cf911f71a0
```

The same integration state can be reproduced from the upstream base with the
twelve ordered mail patches under `patches/snesrecomp/`.

## Patch series

| Patch | Source commit | Purpose |
| --- | --- | --- |
| `0001-Recognize-direct-long-call-trampolines.patch` | `be3a03d` | Treat source-assembled long-call forms as calls and emit the correct return frame. |
| `0002-Fix-open-bus-polling-and-quiescent-resume.patch` | `dc5ee06` | Correct RDNMI open-bus bits and resume parked polling loops at the hardware read. |
| `0003-Prefer-active-interpreter-ownership-for-mixed-tier-r.patch` | `152c8d0` | Give an active interpreter continuation priority over a coincident generated ancestor. |
| `0004-Decode-dispatch-helpers-at-observed-entry-widths.patch` | `028450f` | Classify dispatch helpers at observed M/X widths and retract disagreements. |
| `0005-Add-shared-binary-input-replay-reader.patch` | `1ce52cc` | Validate and play the shared two-controller `.sri` replay format. |
| `0006-Retract-stale-exit-facts-after-unresolved-paths.patch` | `b1453e9` | Retract inferred exit facts when later analysis exposes unresolved execution. |
| `0007-Expose-interpreted-RTI-completion-hooks.patch` | `303ced1` | Let event-driven hosts observe an exact interpreted RTI continuation. |
| `0008-Audit-generated-charges-crossing-event-deadlines.patch` | `d44bda6` | Report generated clock charges that span host event deadlines. |
| `0009-Add-audit-guided-event-precision-path.patch` | `0e559d2` | Route only affected generated variants through instruction-timed execution. |
| `0010-Replay-Mode-7-raster-state-in-frame-hosts.patch` | `503c79c` | Replay scanline-timed display state from generated and emulated writes. |
| `0011-Preserve-BG3-raster-character-addressing.patch` | `e70e901` | Include active-display BG34NBA changes in raster replay. |
| `0012-Deliver-delayed-enable-NMI-requests.patch` | `3678d0a` | Raise a pending NMI when software enables it during active vblank. |

The patches contain game-neutral implementation and synthetic or ROM-free
tests. They do not contain the Super Tennis ROM, generated title code, title
configuration, or private evidence.

## Integrity

The expected SHA-256 values are:

```text
958cafffd9de2c0ddc7e70806ed0e1f88a996a4f1286d68b301677681df5b88e  0001
21c4cce8e25e8bf36dc37cff3d04976bc78d266340bcad76a01d196dbdeb9cd0  0002
bcdb19fc70054aed8d5bd954052c10ef591ab575fd15caeb759842e0c5cb02d0  0003
3a0e26315e7fde43baf504f02c7f729dbadfb6dd9430dbf7add2fae47a020d28  0004
a2ac4b3e6f08aa258416db15a4ab879f6ab2453bb8993349449e54c7d6459149  0005
027cde5c0d18e98a7615946ac333c1eeb84dab84de42b42dd0b456b90d1b8651  0006
c92b3cde3589f1323707c073b4658a58dd9172533d916b3b572103e9540228c1  0007
fe32ded54bffde3f83436bce06a9807c170605c1590853d87bf2e8d3eb0ff965  0008
4c1aa6e2c2e47989deead6d210f046791dcf9ba0367e7b4d73bd90eab9f65721  0009
bbe0d5623aa6d3ba9ed58fc1808b4864741a89ab2c9e36d4af7c30660ea94b7e  0010
29c6af194ea7a79839f2450bd46fa77178771c75a360935acb1e7716911c47f6  0011
b7ab7b58ba8a6fa7dd47b262cfb5988c7e080fd5712855f725b982b21ec243af  0012
```

Verify them from the repository root with:

```sh
sha256sum patches/snesrecomp/*.patch
```

On macOS, use `shasum -a 256` if `sha256sum` is unavailable.

## Applying

```sh
git submodule update --init
sh tools/apply-snesrecomp-patches.sh
```

The script accepts either the pinned public integration commit or the expected
upstream base. It refuses a dirty or unexpected dependency and is safe to run
again when all twelve patch subjects are already present.

## Verification

After changing the dependency or patch series, regenerate and build the title,
then run:

```sh
python3 snesrecomp/tests/v2/run_tests.py
bash snesrecomp/tests/run_c_tests.sh
```

Run the neutral headless check and the relevant private title replay as
described in [`AGENTS.md`](../AGENTS.md). Test totals and replay lengths are
intentionally not recorded here because they change as coverage grows.

## Upstreaming

The integration commit is published on the project owner's fork. The ordered
patch files remain the portable review and recovery form until the required
changes are accepted upstream.

Before replacing the patch series:

1. Rebase each logical change against current snesrecomp upstream.
2. Preserve its synthetic tests.
3. Run the Python, C, and relevant Rust suites.
4. Submit the changes through normal upstream review.
5. Pin this repository to a public commit containing every required change.
6. Rerun the title regressions before deleting the superseded patches and
   application script.
