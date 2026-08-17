(* Execution-only Wolfram Language bridge for the preregistered Maeda calibration.
   Scientific authority remains the frozen Python method tree:
   47fa9fbc3ef0f5b1c2a5e91a7ab22efb7088d43c

   This file ports only the frozen composition-facing structural extractor and
   similarity equation so exact private VGM/VGZ blobs can be reduced on a
   networked Wolfram kernel when GitHub Actions/local network transport is
   unavailable. It does not alter labels, gates, feature definitions, or the
   held-out Sonic 3 policy. Every input must match its frozen Git blob SHA.
*)

ClearAll[HelixVGMStructuralExtract, HelixHistogramCosine, HelixStructuralSimilarity];

HelixVGMStructuralExtract[file_, expectedGitBlobSHA_, compressed_: Automatic] :=
 Module[
  {packed, raw, gitsha, read16, read32, dataOffset, pos, size, tick = 0,
   lastTick = 0, pitchLow = ConstantArray[0, 6],
   pitchHigh = ConstantArray[0, 6], op = ConstantArray[0, {6, 4, 7}],
   af = ConstantArray[0, 6], route = ConstantArray[0, 6], dac = False,
   ch3 = False, onsets = {}, psg = 0, noise = 0, stereo = 0, lfo = 0,
   panChanges = 0, dacEnable = 0, dacStream = 0,
   lastPan = ConstantArray[0, 6],
   chanMap = <|0 -> 1, 1 -> 2, 2 -> 3, 4 -> 4, 5 -> 5, 6 -> 6|>,
   groups = {16^^30, 16^^40, 16^^50, 16^^60, 16^^70, 16^^80, 16^^90},
   addOnset, update, fp, intervals = {}, bigrams = {}, contours = {},
   gapsNorm = {}, bychan, hist, fmtQuarter, stream, useGzip},

  stream = OpenRead[file, BinaryFormat -> True];
  packed = BinaryReadList[stream, "Byte"];
  Close[stream];
  gitsha = IntegerString[
    Hash[
     ByteArray@Join[
       ToCharacterCode[
        "blob " <> ToString[Length[packed]] <> FromCharacterCode[0],
        "ISO8859-1"], packed], "SHA1"], 16, 40];

  If[gitsha =!= expectedGitBlobSHA,
   Return[<|"git_match" -> False, "git_sha" -> gitsha,
     "expected_git_sha" -> expectedGitBlobSHA,
     "packed_bytes" -> Length[packed]|>]];

  useGzip = If[compressed === Automatic,
    Length[packed] >= 2 && Take[packed, 2] === {31, 139}, TrueQ[compressed]];
  raw = If[useGzip, Quiet@Import[file, {"GZIP", "Byte"}], packed];

  If[Length[raw] < 64 || Take[raw, 4] =!= {16^^56, 16^^67, 16^^6D, 16^^20},
   Return[<|"git_match" -> True, "git_sha" -> gitsha,
     "error" -> "invalid_vgm_magic"|>]];

  read16[b_, i_] := b[[i]] + 256*b[[i + 1]];
  read32[b_, i_] := b[[i]] + 256*b[[i + 1]] + 65536*b[[i + 2]] +
    16777216*b[[i + 3]];
  dataOffset[b_] := Module[{v = read32[b, 9], r},
    If[v < 16^^150, 65,
     r = read32[b, 53]; If[r == 0, 65, 53 + r]]];

  fp[v_] := StringTake[
    IntegerString[Hash[ByteArray[Mod[v, 256]], "SHA1"], 16, 40], 16];

  update[port_, reg_, val_] :=
   Module[{c, hi, loc, ci, slot, gi},
    If[port == 0 && reg == 16^^2B,
     dac = BitAnd[val, 16^^80] != 0; Return[]];
    If[port == 0 && reg == 16^^27,
     ch3 = BitAnd[val, 16^^40] != 0; Return[]];
    If[16^^A0 <= reg <= 16^^A2,
     c = (reg - 16^^A0) + 1 + 3*port; pitchLow[[c]] = val; Return[]];
    If[16^^A4 <= reg <= 16^^A6,
     c = (reg - 16^^A4) + 1 + 3*port; pitchHigh[[c]] = val; Return[]];
    If[16^^B0 <= reg <= 16^^B2,
     c = (reg - 16^^B0) + 1 + 3*port; af[[c]] = val; Return[]];
    If[16^^B4 <= reg <= 16^^B6,
     c = (reg - 16^^B4) + 1 + 3*port; route[[c]] = val; Return[]];
    hi = BitAnd[reg, 16^^F0];
    If[! MemberQ[groups, hi], Return[]];
    loc = BitAnd[reg, 15]; ci = BitAnd[loc, 3];
    If[ci == 3, Return[]];
    slot = BitAnd[BitShiftRight[loc, 2], 3] + 1;
    c = ci + 1 + 3*port;
    gi = First@FirstPosition[groups, hi];
    op[[c, slot, gi]] = val;
    ];

  addOnset[t_, c_] :=
   Module[{hi, fn, bl, av, rv, algorithm, feedback, ams, fms, pan, cv, fv},
    If[(c == 6 && dac) || (c == 3 && ch3), Return[]];
    hi = pitchHigh[[c]];
    fn = BitOr[BitShiftLeft[BitAnd[hi, 7], 8], pitchLow[[c]]];
    bl = BitAnd[BitShiftRight[hi, 3], 7];
    If[fn == 0, Return[]];
    av = af[[c]]; rv = route[[c]];
    algorithm = BitAnd[av, 7];
    feedback = BitAnd[BitShiftRight[av, 3], 7];
    ams = BitAnd[BitShiftRight[rv, 4], 3];
    fms = BitAnd[rv, 7];
    pan = BitAnd[BitShiftRight[rv, 6], 3];
    cv = {algorithm, feedback};
    fv = {algorithm, feedback, ams, fms};
    Do[
     cv = Join[cv, op[[c, s, {1, 3, 4, 5, 6, 7}]]];
     fv = Join[fv, op[[c, s, All]]], {s, 4}];
    AppendTo[onsets,
     <|"tick" -> t, "channel" -> (c - 1), "fnum" -> fn,
      "block" -> bl|>];
    ];

  pos = dataOffset[raw]; size = Length[raw];
  While[pos <= size,
   Module[{opcode = raw[[pos]], reg, val, port, wait, block, enc, mask, c},
    pos++;
    Which[
     opcode == 16^^66, pos = size + 1,
     MemberQ[{16^^52, 16^^53}, opcode],
     If[pos + 1 > size,
      Return[<|"git_match" -> True, "git_sha" -> gitsha,
        "error" -> "truncated_ym_write"|>]];
     reg = raw[[pos]]; val = raw[[pos + 1]]; pos += 2;
     lastTick = Max[lastTick, tick]; port = If[opcode == 16^^52, 0, 1];
     If[port == 0 && reg == 16^^22, lfo++];
     If[port == 0 && reg == 16^^2B, dacEnable++];
     If[16^^B4 <= reg <= 16^^B6,
      c = (reg - 16^^B4) + 1 + 3*port;
      If[BitAnd[BitShiftRight[val, 6], 3] != lastPan[[c]],
       panChanges++; lastPan[[c]] = BitAnd[BitShiftRight[val, 6], 3]]];
     update[port, reg, val];
     If[port == 0 && reg == 16^^28,
      mask = BitAnd[val, 16^^F0]; enc = BitAnd[val, 7];
      If[mask == 16^^F0 && KeyExistsQ[chanMap, enc],
       addOnset[tick, chanMap[enc]]]],

     MemberQ[{16^^4F, 16^^50}, opcode],
     If[pos > size,
      Return[<|"git_match" -> True, "git_sha" -> gitsha,
        "error" -> "truncated_psg"|>]];
     val = raw[[pos]]; pos++; lastTick = Max[lastTick, tick];
     If[opcode == 16^^50, psg++;
      If[BitAnd[val, 16^^F0] == 16^^E0, noise++], stereo++],

     opcode == 16^^61,
     If[pos + 1 > size,
      Return[<|"git_match" -> True, "git_sha" -> gitsha,
        "error" -> "truncated_wait"|>]];
     wait = read16[raw, pos]; pos += 2; tick += wait,
     opcode == 16^^62, tick += 735,
     opcode == 16^^63, tick += 882,
     16^^70 <= opcode <= 16^^7F, tick += BitAnd[opcode, 15] + 1,
     16^^80 <= opcode <= 16^^8F,
     lastTick = Max[lastTick, tick]; dacStream++; tick += BitAnd[opcode, 15],
     opcode == 16^^67,
     If[pos + 5 > size || raw[[pos]] != 16^^66,
      Return[<|"git_match" -> True, "git_sha" -> gitsha,
        "error" -> "malformed_data_block"|>]];
     block = read32[raw, pos + 2]; pos += 6 + block,
     opcode == 16^^E0, pos += 4,
     MemberQ[{16^^90, 16^^91}, opcode],
     lastTick = Max[lastTick, tick]; dacStream++; pos += 4,
     opcode == 16^^92,
     lastTick = Max[lastTick, tick]; dacStream++; pos += 5,
     opcode == 16^^93,
     lastTick = Max[lastTick, tick]; dacStream++; pos += 10,
     opcode == 16^^94,
     lastTick = Max[lastTick, tick]; dacStream++; pos += 1,
     opcode == 16^^95,
     lastTick = Max[lastTick, tick]; dacStream++; pos += 4,
     True,
     Return[<|"git_match" -> True, "git_sha" -> gitsha,
       "error" -> ("unsupported_opcode_" <> IntegerString[opcode, 16, 2])|>]
     ]
    ]
   ];

  bychan = GatherBy[onsets, #channel &];
  fmtQuarter[q_] := ToString[Quotient[q, 4]] <>
    Switch[Mod[q, 4], 0, ".00", 1, ".25", 2, ".50", 3, ".75"];

  Do[
   Module[{ints = {}, events = ev, gg, med, q, semi, rnd, label, num},
    Do[
     semi = 12*Log[2,
        (events[[i + 1, "fnum"]]*2.^events[[i + 1, "block"]])/
         (events[[i, "fnum"]]*2.^events[[i, "block"]])];
     rnd = Round[semi];
     label = If[rnd < -24, "<-24", If[rnd > 24, ">24", ToString[rnd]]];
     AppendTo[ints, label]; AppendTo[intervals, label];
     num = If[MemberQ[{"<-24", ">24"}, label], None, rnd];
     AppendTo[contours,
      If[label == ">24" || (num =!= None && num > 0), "up",
       If[label == "<-24" || (num =!= None && num < 0), "down", "same"]]],
     {i, Length[events] - 1}];
    Do[AppendTo[bigrams, ints[[i]] <> "," <> ints[[i + 1]]],
     {i, Max[0, Length[ints] - 1]}];
    gg = Select[
      Table[events[[i + 1, "tick"]] - events[[i, "tick"]],
       {i, Length[events] - 1}], # > 0 &];
    If[Length[gg] > 0,
     med = Median[gg];
     If[med > 0,
      Do[q = Round[Min[4., g/med]*4.]; AppendTo[gapsNorm, fmtQuarter[q]],
       {g, gg}]]]
    ], {ev, bychan}];

  hist[x_] := Association@KeySort@Counts[x];
  <|"git_match" -> True, "git_sha" -> gitsha,
   "packed_bytes" -> Length[packed],
   "ordinary_full_fm_key_ons" -> Length[onsets],
   "interval_histogram_semitones" -> hist[intervals],
   "interval_bigram_histogram" -> hist[bigrams],
   "contour_histogram" -> hist[contours],
   "normalized_onset_gap_histogram" -> hist[gapsNorm]|>
  ];

HelixHistogramCosine[a_Association, b_Association] :=
 Module[{keys, va, vb, na, nb},
  If[Length[a] == 0 || Length[b] == 0, Return[Missing["NA"]]];
  keys = Union[Keys[a], Keys[b]];
  va = N[Lookup[a, keys, 0]]; vb = N[Lookup[b, keys, 0]];
  na = Sqrt[va.va]; nb = Sqrt[vb.vb];
  If[na == 0 || nb == 0, Missing["NA"], N[(va.vb)/(na*nb), 17]]
  ];

HelixStructuralSimilarity[a_Association, b_Association] :=
 Module[{vals},
  vals = DeleteCases[
    {HelixHistogramCosine[a["interval_histogram_semitones"],
      b["interval_histogram_semitones"]],
     HelixHistogramCosine[a["interval_bigram_histogram"],
      b["interval_bigram_histogram"]],
     HelixHistogramCosine[a["normalized_onset_gap_histogram"],
      b["normalized_onset_gap_histogram"]],
     HelixHistogramCosine[a["contour_histogram"], b["contour_histogram"]]},
    _Missing];
  If[vals === {}, 0., Mean[vals]]
  ];
