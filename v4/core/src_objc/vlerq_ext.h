/* vlerq_ext.h - generated code, do not edit */

/* 79 definitions generated for Objective-C:

  # name       in     out    inline-code
  : drop       U      {}     {}
  : dup        U      UU     {$1 = $0;}
  : imul       II     I      {$0.i *=  $1.i;}
  : nip        UU     U      {$0 = $1;}
  : over       UU     UUU    {$2 = $0;}
  : rot        UUU    UUU    {$3 = $0; $0 = $1; $1 = $2; $2 = $3;}
  : rrot       UUU    UUU    {$3 = $2; $2 = $1; $1 = $0; $0 = $3;}
  : swap       UU     UU     {$2 = $0; $0 = $1; $1 = $2;}
  : tuck       UU     UUU    {$1 = $2; $0 = $1; $2 = $0;}
  
  tcl {
    # name       in    out
    : Def        OO     V
    : Deps       O      O
    : Emit       V      O
    : EmitMods   V      O
    : Get        VX     U
    : Load       O      V
    : LoadMods   OV     V
    : Loop       X      O
    : MutInfo    V      O
    : Ref        OX     O
    : Refs       O      I
    : Rename     VO     V
    : Save       VS     I
    : To         OO     O
    : Type       O      O
    : View       X      O
  }
  
  python {
    # name       in    out
  }
  
  ruby {
    # name       in    out
    : AtRow      OI     O
  }
  
  lua {
    # name       in    out
  }
  
  objc {
    # name       in    out
    : At         VIO    O
  }
  
  # name       in    out
  : BitRuns    i      C
  : Data       VX     V
  : Debug      I      I
  : HashCol    SO     C
  : Max        VN     O
  : Min        VN     O
  : Open       S      V
  : ResizeCol  iII    C
  : Set        MIX    V
  : StructDesc V      S
  : Structure  V      S
  : Sum        VN     O
  : Write      VO     I

  # name       in    out  internal-name
  : Blocked    V      V   BlockedView
  : Clone      V      V   CloneView
  : ColMap     Vn     V   ColMapView
  : ColOmit    Vn     V   ColOmitView
  : Coerce     OS     C   CoerceCmd
  : Compare    VV     I   ViewCompare
  : Compat     VV     I   ViewCompat
  : Concat     VV     V   ConcatView
  : CountsCol  C      C   NewCountsColumn
  : CountView  I      V   NoColumnView
  : First      VI     V   FirstView
  : GetCol     VN     C   ViewCol
  : Group      VnS    V   GroupCol
  : HashFind   VIViii I   HashDoFind
  : Ijoin      VV     V   IjoinView
  : GetInfo    VVI    C   GetHashInfo
  : Grouped    ViiS   V   GroupedView
  : HashView   V      C   HashValues
  : IsectMap   VV     C   IntersectMap
  : Iota       I      C   NewIotaColumn
  : Join       VVS    V   JoinView
  : Last       VI     V   LastView
  : Mdef       O      V   ObjAsMetaView
  : Mdesc      S      V   DescToMeta
  : Meta       V      V   V_Meta
  : OmitMap    iI     C   OmitColumn
  : OneCol     VN     V   OneColView
  : Pair       VV     V   PairView
  : RemapSub   ViII   V   RemapSubview
  : Replace    VIIV   V   ViewReplace
  : RowCmp     VIVI   I   RowCompare
  : RowEq      VIVI   I   RowEqual
  : RowHash    VI     I   RowHash
  : Size       V      I   ViewSize
  : SortMap    V      C   SortMap
  : Step       VIIII  V   StepView
  : StrLookup  Ss     I   StringLookup
  : Tag        VS     V   TagView
  : Take       VI     V   TakeView
  : Ungroup    VN     V   UngroupView
  : UniqueMap  V      C   UniqMap
  : ViewAsCol  V      C   ViewAsCol
  : Width      V      I   ViewWidth
               
  # name       in    out  compound-definition
  : Append     VV     V   {over size swap insert}
  : ColConv    C      C   { }
  : Counts     VN     C   {getcol countscol}
  : Delete     VII    V   {0 countview replace}
  : Except     VV     V   {over swap exceptmap remap}
  : ExceptMap  VV     C   {over swap isectmap swap size omitmap}
  : Insert     VIV    V   {0 swap replace}
  : Intersect  VV     V   {over swap isectmap remap}
  : NameCol    V      V   {meta 0 onecol}
  : Names      V      C   {meta 0 getcol}
  : Product    VV     V   {over over size spread rrot swap size repeat pair}
  : Repeat     VI     V   {over size imul 0 1 1 step}
  : Remap      Vi     V   {0 -1 remapsub}
  : Reverse    V      V   {dup size -1 1 -1 step}
  : Slice      VIII   V   {rrot 1 swap step}
  : Sort       V      V   {dup sortmap remap}
  : Spread     VI     V   {over size 0 rot 1 step}
  : Types      V      C   {meta 1 getcol}
  : Unique     V      V   {dup uniquemap remap}
  : Union      VV     V   {over except concat}
  : UnionMap   VV     C   {swap exceptmap}
  : ViewConv   V      V   { }
  
  # some operators are omitted from restricted execution environments
  unsafe Open Save
*/

  { "append",     "V:VV",     AppendCmd_VV       },
  { "at",         "O:VIO",    AtCmd_VIO          },
  { "bitruns",    "C:i",      BitRunsCmd_i       },
  { "blocked",    "V:V",      BlockedCmd_V       },
  { "clone",      "V:V",      CloneCmd_V         },
  { "coerce",     "C:OS",     CoerceCmd_OS       },
  { "colconv",    "C:C",      ColConvCmd_C       },
  { "colmap",     "V:Vn",     ColMapCmd_Vn       },
  { "colomit",    "V:Vn",     ColOmitCmd_Vn      },
  { "compare",    "I:VV",     CompareCmd_VV      },
  { "compat",     "I:VV",     CompatCmd_VV       },
  { "concat",     "V:VV",     ConcatCmd_VV       },
  { "counts",     "C:VN",     CountsCmd_VN       },
  { "countscol",  "C:C",      CountsColCmd_C     },
  { "countview",  "V:I",      CountViewCmd_I     },
  { "data",       "V:VX",     DataCmd_VX         },
  { "debug",      "I:I",      DebugCmd_I         },
  { "delete",     "V:VII",    DeleteCmd_VII      },
  { "except",     "V:VV",     ExceptCmd_VV       },
  { "exceptmap",  "C:VV",     ExceptMapCmd_VV    },
  { "first",      "V:VI",     FirstCmd_VI        },
  { "getcol",     "C:VN",     GetColCmd_VN       },
  { "getinfo",    "C:VVI",    GetInfoCmd_VVI     },
  { "group",      "V:VnS",    GroupCmd_VnS       },
  { "grouped",    "V:ViiS",   GroupedCmd_ViiS    },
  { "hashcol",    "C:SO",     HashColCmd_SO      },
  { "hashfind",   "I:VIViii", HashFindCmd_VIViii },
  { "hashview",   "C:V",      HashViewCmd_V      },
  { "ijoin",      "V:VV",     IjoinCmd_VV        },
  { "insert",     "V:VIV",    InsertCmd_VIV      },
  { "intersect",  "V:VV",     IntersectCmd_VV    },
  { "iota",       "C:I",      IotaCmd_I          },
  { "isectmap",   "C:VV",     IsectMapCmd_VV     },
  { "join",       "V:VVS",    JoinCmd_VVS        },
  { "last",       "V:VI",     LastCmd_VI         },
  { "max",        "O:VN",     MaxCmd_VN          },
  { "mdef",       "V:O",      MdefCmd_O          },
  { "mdesc",      "V:S",      MdescCmd_S         },
  { "meta",       "V:V",      MetaCmd_V          },
  { "min",        "O:VN",     MinCmd_VN          },
  { "namecol",    "V:V",      NameColCmd_V       },
  { "names",      "C:V",      NamesCmd_V         },
  { "omitmap",    "C:iI",     OmitMapCmd_iI      },
  { "onecol",     "V:VN",     OneColCmd_VN       },
  { "open",       "V:S",      OpenCmd_S          },
  { "pair",       "V:VV",     PairCmd_VV         },
  { "product",    "V:VV",     ProductCmd_VV      },
  { "remap",      "V:Vi",     RemapCmd_Vi        },
  { "remapsub",   "V:ViII",   RemapSubCmd_ViII   },
  { "repeat",     "V:VI",     RepeatCmd_VI       },
  { "replace",    "V:VIIV",   ReplaceCmd_VIIV    },
  { "resizecol",  "C:iII",    ResizeColCmd_iII   },
  { "reverse",    "V:V",      ReverseCmd_V       },
  { "rowcmp",     "I:VIVI",   RowCmpCmd_VIVI     },
  { "roweq",      "I:VIVI",   RowEqCmd_VIVI      },
  { "rowhash",    "I:VI",     RowHashCmd_VI      },
  { "set",        "V:MIX",    SetCmd_MIX         },
  { "size",       "I:V",      SizeCmd_V          },
  { "slice",      "V:VIII",   SliceCmd_VIII      },
  { "sort",       "V:V",      SortCmd_V          },
  { "sortmap",    "C:V",      SortMapCmd_V       },
  { "spread",     "V:VI",     SpreadCmd_VI       },
  { "step",       "V:VIIII",  StepCmd_VIIII      },
  { "strlookup",  "I:Ss",     StrLookupCmd_Ss    },
  { "structdesc", "S:V",      StructDescCmd_V    },
  { "structure",  "S:V",      StructureCmd_V     },
  { "sum",        "O:VN",     SumCmd_VN          },
  { "tag",        "V:VS",     TagCmd_VS          },
  { "take",       "V:VI",     TakeCmd_VI         },
  { "types",      "C:V",      TypesCmd_V         },
  { "ungroup",    "V:VN",     UngroupCmd_VN      },
  { "union",      "V:VV",     UnionCmd_VV        },
  { "unionmap",   "C:VV",     UnionMapCmd_VV     },
  { "unique",     "V:V",      UniqueCmd_V        },
  { "uniquemap",  "C:V",      UniqueMapCmd_V     },
  { "viewascol",  "C:V",      ViewAsColCmd_V     },
  { "viewconv",   "V:V",      ViewConvCmd_V      },
  { "width",      "I:V",      WidthCmd_V         },
  { "write",      "I:VO",     WriteCmd_VO        },

/* end of generated code */
