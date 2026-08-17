(* Execution-only Wolfram bridge for Maeda Gen-2 validation.
   Scientific definition is frozen in maeda-part-matcher-gen2-preregistration.json.
   This helper performs only creator-blind per-channel feature extraction.
*)

ClearAll[HelixGen2PartExtract, helixRead16, helixRead32];
helixRead16[b_, p_] := FromDigits[Reverse@b[[p ;; p + 1]], 256];
helixRead32[b_, p_] := FromDigits[Reverse@b[[p ;; p + 3]], 256];

HelixGen2PartExtract[file_] := Module[
  {raw, version, relative, pos, size, tick = 0,
   pitchLow = ConstantArray[0, 6], pitchHigh = ConstantArray[0, 6],
   dac = False, ch3 = False, opcode, reg, val, port, wait, blockSize,
   encoded, mask, channel, fnum, block, reaped, events, grouped,
   channelEvents, rounded, intervals, parts = {}},

  raw = If[
    ToLowerCase@FileExtension[file] === "vgz",
    Quiet@Import[file, {"GZIP", "Byte"}],
    BinaryReadList[file, "Byte"]
  ];
  If[!ListQ[raw] || Length[raw] < 64 || Take[raw, 4] =!= {16^^56,16^^67,16^^6D,16^^20},
    Return[<|"error" -> "invalid_vgm"|>]
  ];

  version = helixRead32[raw, 9];
  relative = If[version < 16^^150, 0, helixRead32[raw, 53]];
  pos = If[version < 16^^150 || relative == 0, 65, 53 + relative];
  size = Length[raw];

  reaped = Reap[
    While[pos <= size,
      opcode = raw[[pos++]];
      Which[
        opcode == 16^^66, Break[],
        opcode == 16^^52 || opcode == 16^^53,
          reg = raw[[pos]]; val = raw[[pos + 1]]; pos += 2;
          port = Boole[opcode == 16^^53];
          If[port == 0 && reg == 16^^2B, dac = BitGet[val, 7] == 1];
          If[port == 0 && reg == 16^^27, ch3 = BitGet[val, 6] == 1];
          If[16^^A0 <= reg <= 16^^A2,
            channel = reg - 16^^A0 + 3 port; pitchLow[[channel + 1]] = val];
          If[16^^A4 <= reg <= 16^^A6,
            channel = reg - 16^^A4 + 3 port; pitchHigh[[channel + 1]] = val];
          If[port == 0 && reg == 16^^28,
            mask = BitAnd[val, 16^^F0]; encoded = BitAnd[val, 7];
            If[mask == 16^^F0 && MemberQ[{0,1,2,4,5,6}, encoded],
              channel = Switch[encoded,0,0,1,1,2,2,4,3,5,4,6,5];
              If[!(channel == 5 && dac) && !(channel == 2 && ch3),
                fnum = BitShiftLeft[BitAnd[pitchHigh[[channel + 1]],7],8] + pitchLow[[channel + 1]];
                block = BitAnd[BitShiftRight[pitchHigh[[channel + 1]],3],7];
                If[fnum != 0, Sow[{channel,tick,N[fnum 2^block]}]]
              ]
            ]
          ],
        opcode == 16^^4F || opcode == 16^^50, pos += 1,
        opcode == 16^^61, wait = helixRead16[raw,pos]; pos += 2; tick += wait,
        opcode == 16^^62, tick += 735,
        opcode == 16^^63, tick += 882,
        16^^70 <= opcode <= 16^^7F, tick += BitAnd[opcode,15] + 1,
        16^^80 <= opcode <= 16^^8F, tick += BitAnd[opcode,15],
        opcode == 16^^67, blockSize = helixRead32[raw,pos + 2]; pos += 6 + blockSize,
        opcode == 16^^E0, pos += 4,
        opcode == 16^^90 || opcode == 16^^91 || opcode == 16^^95, pos += 4,
        opcode == 16^^92, pos += 5,
        opcode == 16^^93, pos += 10,
        opcode == 16^^94, pos += 1,
        True, Return[<|"error" -> ("unsupported_opcode_" <> IntegerString[opcode,16,2])|>]
      ]
    ]
  ];

  events = If[Length[reaped[[2]]] > 0, First[reaped[[2]]], {}];
  grouped = GroupBy[events, First -> Rest];
  Do[
    channelEvents = Lookup[grouped, channel, {}];
    If[Length[channelEvents] >= 2,
      rounded = Round /@ (12. Log[2, Rest[channelEvents[[All,2]]] / Most[channelEvents[[All,2]]]]);
      intervals = Map[Which[# < -24,"<-24",# > 24,">24",True,ToString[#]] &, rounded];
      If[intervals =!= {},
        AppendTo[parts, <|
          "channel" -> channel,
          "key_ons" -> Length[channelEvents],
          "interval_histogram_semitones" -> Association@KeySort@Counts[intervals],
          "interval_bigram_histogram" -> Association@KeySort@Counts[
            If[Length[intervals] >= 2, StringRiffle[#,","] & /@ Partition[intervals,2,1], {}]
          ]
        |>]
      ]
    ],
    {channel,0,5}
  ];

  <|"file" -> FileNameTake[file], "parts" -> parts|>
];

HelixGen2PackExtract[zipUrl_, mapping_Association] := Module[
  {zip, dir, files, byNumber, features, json, tmp},
  zip = CreateTemporary[];
  URLDownload[zipUrl, zip];
  dir = CreateDirectory[];
  Quiet@ExtractArchive[zip, dir];
  files = FileNames[{"*.vgm","*.vgz"}, dir, Infinity];
  byNumber = Association@Map[(StringTake[FileNameTake[#],2] -> #) &, files];
  features = Association@KeyValueMap[
    (#1 -> HelixGen2PartExtract[byNumber[#2]]) &,
    mapping
  ];
  json = ExportString[features, "RawJSON"];
  tmp = CreateTemporary[];
  Export[tmp, json, "GZIP"];
  BaseEncode[ByteArray[BinaryReadList[tmp,"Byte"]]]
];
