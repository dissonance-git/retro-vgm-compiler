(* Execution-only Wolfram Language bridge for the preregistered Maeda calibration.
   Scientific authority remains the frozen Python method tree:
   47fa9fbc3ef0f5b1c2a5e91a7ab22efb7088d43c

   This bridge ports only the frozen composition-facing structural extractor and
   similarity equation for environments where the literal Python runner is not
   available. Repository/public-mirror files are treated as the supplied input;
   routine execution does not hash them. Validate corpus provenance when files
   are added or changed, not on every analysis run.

   It does not alter labels, gates, feature definitions, or the held-out Sonic 3
   policy. Invalid/truncated VGM data still fails closed during parsing.
*)

ClearAll[
  HelixVGMStructuralExtract,
  HelixHistogramCosine,
  HelixStructuralSimilarity,
  HelixStructuralPitchSimilarity,
  HelixStructuralRhythmSimilarity
];

HelixVGMStructuralExtract[file_, compressed_: Automatic] := Module[
  {packed, raw, read16, read32, pos, size, tick = 0,
   pitchLow = ConstantArray[0, 6], pitchHigh = ConstantArray[0, 6],
   dac = False, ch3 = False, opcode, reg, val, port, wait, blockSize,
   encoded, mask, channel, fnum, block, reaped, events, byChannel,
   channelEvents, ratios, rounded, intervalLabels, gaps, medianGap,
   intervals = {}, bigrams = {}, contours = {}, normalizedGaps = {},
   useGzip, hist, fmtQuarter},

  packed = BinaryReadList[file, "Byte"];
  useGzip = If[compressed === Automatic,
    Length[packed] >= 2 && Take[packed, 2] === {31, 139},
    TrueQ[compressed]
  ];
  raw = If[useGzip, Quiet@Import[file, {"GZIP", "Byte"}], packed];

  If[!ListQ[raw] || Length[raw] < 64 ||
     Take[raw, 4] =!= {16^^56, 16^^67, 16^^6D, 16^^20},
    Return[<|"error" -> "invalid_vgm_magic", "packed_bytes" -> Length[packed]|>]
  ];

  read16[b_, i_] := FromDigits[Reverse@b[[i ;; i + 1]], 256];
  read32[b_, i_] := FromDigits[Reverse@b[[i ;; i + 3]], 256];

  pos = Module[{version = read32[raw, 9], relative},
    If[version < 16^^150, 65,
      relative = read32[raw, 53];
      If[relative == 0, 65, 53 + relative]
    ]
  ];
  size = Length[raw];

  reaped = Reap[
    While[pos <= size,
      opcode = raw[[pos++]];
      Which[
        opcode == 16^^66,
          Break[],

        opcode == 16^^52 || opcode == 16^^53,
          If[pos + 1 > size, Return[<|"error" -> "truncated_ym_write"|>]];
          reg = raw[[pos]]; val = raw[[pos + 1]]; pos += 2;
          port = Boole[opcode == 16^^53];

          If[port == 0 && reg == 16^^2B, dac = BitGet[val, 7] == 1];
          If[port == 0 && reg == 16^^27, ch3 = BitGet[val, 6] == 1];

          If[16^^A0 <= reg <= 16^^A2,
            channel = reg - 16^^A0 + 3 port;
            pitchLow[[channel + 1]] = val
          ];
          If[16^^A4 <= reg <= 16^^A6,
            channel = reg - 16^^A4 + 3 port;
            pitchHigh[[channel + 1]] = val
          ];

          If[port == 0 && reg == 16^^28,
            mask = BitAnd[val, 16^^F0];
            encoded = BitAnd[val, 7];
            If[mask == 16^^F0 && MemberQ[{0, 1, 2, 4, 5, 6}, encoded],
              channel = Switch[encoded, 0, 0, 1, 1, 2, 2, 4, 3, 5, 4, 6, 5];
              If[!(channel == 5 && dac) && !(channel == 2 && ch3),
                fnum = BitShiftLeft[BitAnd[pitchHigh[[channel + 1]], 7], 8] +
                  pitchLow[[channel + 1]];
                block = BitAnd[BitShiftRight[pitchHigh[[channel + 1]], 3], 7];
                If[fnum != 0, Sow[{channel, tick, N[fnum 2^block]}]]
              ]
            ]
          ],

        opcode == 16^^4F || opcode == 16^^50,
          If[pos > size, Return[<|"error" -> "truncated_psg"|>]];
          pos += 1,

        opcode == 16^^61,
          If[pos + 1 > size, Return[<|"error" -> "truncated_wait"|>]];
          wait = read16[raw, pos]; pos += 2; tick += wait,
        opcode == 16^^62, tick += 735,
        opcode == 16^^63, tick += 882,
        16^^70 <= opcode <= 16^^7F, tick += BitAnd[opcode, 15] + 1,
        16^^80 <= opcode <= 16^^8F, tick += BitAnd[opcode, 15],

        opcode == 16^^67,
          If[pos + 5 > size || raw[[pos]] != 16^^66,
            Return[<|"error" -> "malformed_data_block"|>]
          ];
          blockSize = read32[raw, pos + 2]; pos += 6 + blockSize,

        opcode == 16^^E0, pos += 4,
        opcode == 16^^90 || opcode == 16^^91 || opcode == 16^^95, pos += 4,
        opcode == 16^^92, pos += 5,
        opcode == 16^^93, pos += 10,
        opcode == 16^^94, pos += 1,

        True,
          Return[<|"error" -> ("unsupported_opcode_" <>
            IntegerString[opcode, 16, 2])|>]
      ]
    ]
  ];

  events = If[Length[reaped[[2]]] > 0, First[reaped[[2]]], {}];
  byChannel = GroupBy[events, First -> Rest];
  fmtQuarter[q_] := ToString@NumberForm[
    q, {Infinity, 2}, NumberPadding -> {"", "0"}
  ];

  Do[
    channelEvents = Lookup[byChannel, channel, {}];
    If[Length[channelEvents] >= 2,
      ratios = Rest[channelEvents[[All, 2]]] / Most[channelEvents[[All, 2]]];
      rounded = Round /@ (12. Log[2, ratios]);
      intervalLabels = Map[
        Which[# < -24, "<-24", # > 24, ">24", True, ToString[#]] &,
        rounded
      ];
      intervals = Join[intervals, intervalLabels];
      contours = Join[contours, Map[
        Which[# > 0, "up", # < 0, "down", True, "same"] &,
        rounded
      ]];
      If[Length[intervalLabels] >= 2,
        bigrams = Join[bigrams,
          StringRiffle[#, ","] & /@ Partition[intervalLabels, 2, 1]
        ]
      ];

      gaps = Select[Differences[channelEvents[[All, 1]]], # > 0 &];
      If[gaps =!= {},
        medianGap = Median[gaps];
        If[medianGap > 0,
          normalizedGaps = Join[normalizedGaps,
            fmtQuarter /@ ((Round[Min[4., N[#/medianGap]] 4.]/4.) & /@ gaps)
          ]
        ]
      ]
    ],
    {channel, 0, 5}
  ];

  hist[x_] := Association@KeySort@Counts[x];
  <|
    "packed_bytes" -> Length[packed],
    "ordinary_full_fm_key_ons" -> Length[events],
    "interval_histogram_semitones" -> hist[intervals],
    "interval_bigram_histogram" -> hist[bigrams],
    "contour_histogram" -> hist[contours],
    "normalized_onset_gap_histogram" -> hist[normalizedGaps]
  |>
];

HelixHistogramCosine[a_Association, b_Association] := Module[
  {keys, va, vb, na, nb},
  keys = Union[Keys[a], Keys[b]];
  If[keys === {}, Return[Missing["NA"]]];
  va = N[Lookup[a, keys, 0]];
  vb = N[Lookup[b, keys, 0]];
  na = Sqrt[va.va]; nb = Sqrt[vb.vb];
  If[na == 0 || nb == 0, Missing["NA"], N[(va.vb)/(na nb), 17]]
];

HelixStructuralSimilarity[a_Association, b_Association] := Module[{vals},
  vals = DeleteCases[{
    HelixHistogramCosine[a["interval_histogram_semitones"], b["interval_histogram_semitones"]],
    HelixHistogramCosine[a["interval_bigram_histogram"], b["interval_bigram_histogram"]],
    HelixHistogramCosine[a["normalized_onset_gap_histogram"], b["normalized_onset_gap_histogram"]],
    HelixHistogramCosine[a["contour_histogram"], b["contour_histogram"]]
  }, _Missing];
  If[vals === {}, 0., Mean[vals]]
];

HelixStructuralPitchSimilarity[a_Association, b_Association] := Module[{vals},
  vals = DeleteCases[{
    HelixHistogramCosine[a["interval_histogram_semitones"], b["interval_histogram_semitones"]],
    HelixHistogramCosine[a["interval_bigram_histogram"], b["interval_bigram_histogram"]],
    HelixHistogramCosine[a["contour_histogram"], b["contour_histogram"]]
  }, _Missing];
  If[vals === {}, 0., Mean[vals]]
];

HelixStructuralRhythmSimilarity[a_Association, b_Association] := Module[{value},
  value = HelixHistogramCosine[
    a["normalized_onset_gap_histogram"],
    b["normalized_onset_gap_histogram"]
  ];
  If[MissingQ[value], 0., value]
];
