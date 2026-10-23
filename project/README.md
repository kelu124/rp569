# project/ — reference projects

Third-party or earlier projects kept as reference material (e.g. `phantomdrive`).
Read-only for this repo: do not edit them, copy what is needed into `hardware/` or `docs/`.

| Folder | Source / URL | Why it's here |
|--------|--------------|---------------|
| _(phantomdrive — to add)_ | | |
| `hydrausb3/hydrausb3_hw` | github.com/hydrausb3/hydrausb3_hw @ 9aa0698 (2024-01-22) | CH569 dev-kit reference: schematic PDF, **HSPI/SerDes/UART pin maps (.ods)**, FreeCAD model. No KiCad sources. No license stated. |
| `hydrausb3/hydrausb3_fw` | github.com/hydrausb3/hydrausb3_fw @ 88879fa (2023-04-25) | CH569 firmware examples (USB3, HSPI). Apache-2.0. |
| `hydrausb3/wch-ch56x-lib` | github.com/hydrausb3/wch-ch56x-lib @ 3b056b5 (2025-02-24) | Tested USB3/USB2/HSPI/SerDes drivers — base for CH569 firmware. Apache-2.0. |
| `hydrausb3/wch-ch56x-bsp` | github.com/hydrausb3/wch-ch56x-bsp @ 8ac96df (2024-03-04) | CH56x BSP (WCH SDK, startup, linker). Apache-2.0. |
| `hydrausb3/wch-ch56x-isp` | github.com/hydrausb3/wch-ch56x-isp @ ff23baa (2026-01-28) | USB ISP flashing tool for CH569 — candidate for automated flashing. No license stated. |

Copies are snapshots (shallow clone, `.git` removed). To refresh: re-clone and update the hash above.
`hydrausb3_hw` and `wch-ch56x-isp` have no license file — fine for private reference, check before publishing this repo.
