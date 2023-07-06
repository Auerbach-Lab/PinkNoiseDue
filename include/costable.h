#define COS_TABLE_SIZE 2048
#define COS_TABLE_AMPLITUDE 1000 //maximum amplitude in table

//first quarter of cosine (0-90 degrees)
static const uint16_t PROGMEM cosTable[COS_TABLE_SIZE] = {
1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 
1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 999,  999,  
999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  999,  
999,  999,  999,  998,  998,  998,  998,  998,  998,  998,  998,  998,  998,  998,  998,  998,  
998,  998,  997,  997,  997,  997,  997,  997,  997,  997,  997,  997,  997,  997,  996,  996,  
996,  996,  996,  996,  996,  996,  996,  996,  995,  995,  995,  995,  995,  995,  995,  995,  
995,  994,  994,  994,  994,  994,  994,  994,  994,  994,  993,  993,  993,  993,  993,  993,  
993,  993,  992,  992,  992,  992,  992,  992,  992,  991,  991,  991,  991,  991,  991,  991,  
990,  990,  990,  990,  990,  990,  989,  989,  989,  989,  989,  989,  989,  988,  988,  988,  
988,  988,  988,  987,  987,  987,  987,  987,  986,  986,  986,  986,  986,  986,  985,  985,  
985,  985,  985,  984,  984,  984,  984,  984,  983,  983,  983,  983,  983,  982,  982,  982,  
982,  982,  981,  981,  981,  981,  981,  980,  980,  980,  980,  980,  979,  979,  979,  979,  
978,  978,  978,  978,  978,  977,  977,  977,  977,  976,  976,  976,  976,  975,  975,  975,  
975,  975,  974,  974,  974,  974,  973,  973,  973,  973,  972,  972,  972,  972,  971,  971,  
971,  971,  970,  970,  970,  969,  969,  969,  969,  968,  968,  968,  968,  967,  967,  967,  
966,  966,  966,  966,  965,  965,  965,  965,  964,  964,  964,  963,  963,  963,  963,  962,  
962,  962,  961,  961,  961,  960,  960,  960,  960,  959,  959,  959,  958,  958,  958,  957,  
957,  957,  956,  956,  956,  956,  955,  955,  955,  954,  954,  954,  953,  953,  953,  952,  
952,  952,  951,  951,  951,  950,  950,  950,  949,  949,  949,  948,  948,  948,  947,  947,  
947,  946,  946,  946,  945,  945,  945,  944,  944,  943,  943,  943,  942,  942,  942,  941,  
941,  941,  940,  940,  940,  939,  939,  938,  938,  938,  937,  937,  937,  936,  936,  935,  
935,  935,  934,  934,  934,  933,  933,  932,  932,  932,  931,  931,  930,  930,  930,  929,  
929,  928,  928,  928,  927,  927,  926,  926,  926,  925,  925,  924,  924,  924,  923,  923,  
922,  922,  922,  921,  921,  920,  920,  920,  919,  919,  918,  918,  917,  917,  917,  916,  
916,  915,  915,  914,  914,  914,  913,  913,  912,  912,  911,  911,  911,  910,  910,  909,  
909,  908,  908,  907,  907,  907,  906,  906,  905,  905,  904,  904,  903,  903,  903,  902,  
902,  901,  901,  900,  900,  899,  899,  898,  898,  897,  897,  897,  896,  896,  895,  895,  
894,  894,  893,  893,  892,  892,  891,  891,  890,  890,  889,  889,  888,  888,  887,  887,  
887,  886,  886,  885,  885,  884,  884,  883,  883,  882,  882,  881,  881,  880,  880,  879,  
879,  878,  878,  877,  877,  876,  876,  875,  875,  874,  874,  873,  873,  872,  872,  871,  
870,  870,  869,  869,  868,  868,  867,  867,  866,  866,  865,  865,  864,  864,  863,  863,  
862,  862,  861,  861,  860,  859,  859,  858,  858,  857,  857,  856,  856,  855,  855,  854,  
854,  853,  852,  852,  851,  851,  850,  850,  849,  849,  848,  848,  847,  846,  846,  845,  
845,  844,  844,  843,  843,  842,  841,  841,  840,  840,  839,  839,  838,  837,  837,  836,  
836,  835,  835,  834,  833,  833,  832,  832,  831,  831,  830,  829,  829,  828,  828,  827,  
827,  826,  825,  825,  824,  824,  823,  823,  822,  821,  821,  820,  820,  819,  818,  818,  
817,  817,  816,  815,  815,  814,  814,  813,  812,  812,  811,  811,  810,  809,  809,  808,  
808,  807,  806,  806,  805,  805,  804,  803,  803,  802,  802,  801,  800,  800,  799,  798,  
798,  797,  797,  796,  795,  795,  794,  794,  793,  792,  792,  791,  790,  790,  789,  789,  
788,  787,  787,  786,  785,  785,  784,  783,  783,  782,  782,  781,  780,  780,  779,  778,  
778,  777,  777,  776,  775,  775,  774,  773,  773,  772,  771,  771,  770,  769,  769,  768,  
767,  767,  766,  766,  765,  764,  764,  763,  762,  762,  761,  760,  760,  759,  758,  758,  
757,  756,  756,  755,  754,  754,  753,  752,  752,  751,  750,  750,  749,  748,  748,  747,  
746,  746,  745,  744,  744,  743,  742,  742,  741,  740,  740,  739,  738,  738,  737,  736,  
736,  735,  734,  734,  733,  732,  732,  731,  730,  730,  729,  728,  728,  727,  726,  725,  
725,  724,  723,  723,  722,  721,  721,  720,  719,  719,  718,  717,  717,  716,  715,  714,  
714,  713,  712,  712,  711,  710,  710,  709,  708,  708,  707,  706,  705,  705,  704,  703,  
703,  702,  701,  701,  700,  699,  698,  698,  697,  696,  696,  695,  694,  693,  693,  692,  
691,  691,  690,  689,  689,  688,  687,  686,  686,  685,  684,  684,  683,  682,  681,  681,  
680,  679,  679,  678,  677,  676,  676,  675,  674,  673,  673,  672,  671,  671,  670,  669,  
668,  668,  667,  666,  666,  665,  664,  663,  663,  662,  661,  660,  660,  659,  658,  658,  
657,  656,  655,  655,  654,  653,  652,  652,  651,  650,  650,  649,  648,  647,  647,  646,  
645,  644,  644,  643,  642,  641,  641,  640,  639,  639,  638,  637,  636,  636,  635,  634,  
633,  633,  632,  631,  630,  630,  629,  628,  627,  627,  626,  625,  624,  624,  623,  622,  
621,  621,  620,  619,  619,  618,  617,  616,  616,  615,  614,  613,  613,  612,  611,  610,  
610,  609,  608,  607,  607,  606,  605,  604,  604,  603,  602,  601,  601,  600,  599,  598,  
598,  597,  596,  595,  595,  594,  593,  592,  592,  591,  590,  589,  589,  588,  587,  586,  
585,  585,  584,  583,  582,  582,  581,  580,  579,  579,  578,  577,  576,  576,  575,  574,  
573,  573,  572,  571,  570,  570,  569,  568,  567,  567,  566,  565,  564,  563,  563,  562,  
561,  560,  560,  559,  558,  557,  557,  556,  555,  554,  554,  553,  552,  551,  551,  550,  
549,  548,  547,  547,  546,  545,  544,  544,  543,  542,  541,  541,  540,  539,  538,  538,  
537,  536,  535,  534,  534,  533,  532,  531,  531,  530,  529,  528,  528,  527,  526,  525,  
525,  524,  523,  522,  521,  521,  520,  519,  518,  518,  517,  516,  515,  515,  514,  513,  
512,  512,  511,  510,  509,  508,  508,  507,  506,  505,  505,  504,  503,  502,  502,  501,  
500,  499,  498,  498,  497,  496,  495,  495,  494,  493,  492,  492,  491,  490,  489,  488,  
488,  487,  486,  485,  485,  484,  483,  482,  482,  481,  480,  479,  479,  478,  477,  476,  
475,  475,  474,  473,  472,  472,  471,  470,  469,  469,  468,  467,  466,  466,  465,  464,  
463,  462,  462,  461,  460,  459,  459,  458,  457,  456,  456,  455,  454,  453,  453,  452,  
451,  450,  449,  449,  448,  447,  446,  446,  445,  444,  443,  443,  442,  441,  440,  440,  
439,  438,  437,  437,  436,  435,  434,  433,  433,  432,  431,  430,  430,  429,  428,  427,  
427,  426,  425,  424,  424,  423,  422,  421,  421,  420,  419,  418,  418,  417,  416,  415,  
415,  414,  413,  412,  411,  411,  410,  409,  408,  408,  407,  406,  405,  405,  404,  403,  
402,  402,  401,  400,  399,  399,  398,  397,  396,  396,  395,  394,  393,  393,  392,  391,  
390,  390,  389,  388,  387,  387,  386,  385,  384,  384,  383,  382,  381,  381,  380,  379,  
379,  378,  377,  376,  376,  375,  374,  373,  373,  372,  371,  370,  370,  369,  368,  367,  
367,  366,  365,  364,  364,  363,  362,  361,  361,  360,  359,  359,  358,  357,  356,  356,  
355,  354,  353,  353,  352,  351,  350,  350,  349,  348,  348,  347,  346,  345,  345,  344,  
343,  342,  342,  341,  340,  340,  339,  338,  337,  337,  336,  335,  334,  334,  333,  332,  
332,  331,  330,  329,  329,  328,  327,  327,  326,  325,  324,  324,  323,  322,  321,  321,  
320,  319,  319,  318,  317,  316,  316,  315,  314,  314,  313,  312,  311,  311,  310,  309,  
309,  308,  307,  307,  306,  305,  304,  304,  303,  302,  302,  301,  300,  299,  299,  298,  
297,  297,  296,  295,  295,  294,  293,  292,  292,  291,  290,  290,  289,  288,  288,  287,  
286,  286,  285,  284,  283,  283,  282,  281,  281,  280,  279,  279,  278,  277,  277,  276,  
275,  275,  274,  273,  272,  272,  271,  270,  270,  269,  268,  268,  267,  266,  266,  265,  
264,  264,  263,  262,  262,  261,  260,  260,  259,  258,  258,  257,  256,  256,  255,  254,  
254,  253,  252,  252,  251,  250,  250,  249,  248,  248,  247,  246,  246,  245,  244,  244,  
243,  242,  242,  241,  240,  240,  239,  238,  238,  237,  236,  236,  235,  234,  234,  233,  
233,  232,  231,  231,  230,  229,  229,  228,  227,  227,  226,  225,  225,  224,  223,  223,  
222,  222,  221,  220,  220,  219,  218,  218,  217,  217,  216,  215,  215,  214,  213,  213,  
212,  211,  211,  210,  210,  209,  208,  208,  207,  206,  206,  205,  205,  204,  203,  203,  
202,  202,  201,  200,  200,  199,  198,  198,  197,  197,  196,  195,  195,  194,  194,  193,  
192,  192,  191,  191,  190,  189,  189,  188,  188,  187,  186,  186,  185,  185,  184,  183,  
183,  182,  182,  181,  180,  180,  179,  179,  178,  177,  177,  176,  176,  175,  175,  174,  
173,  173,  172,  172,  171,  171,  170,  169,  169,  168,  168,  167,  167,  166,  165,  165,  
164,  164,  163,  163,  162,  161,  161,  160,  160,  159,  159,  158,  157,  157,  156,  156,  
155,  155,  154,  154,  153,  152,  152,  151,  151,  150,  150,  149,  149,  148,  148,  147,  
146,  146,  145,  145,  144,  144,  143,  143,  142,  142,  141,  141,  140,  139,  139,  138,  
138,  137,  137,  136,  136,  135,  135,  134,  134,  133,  133,  132,  132,  131,  131,  130,  
130,  129,  128,  128,  127,  127,  126,  126,  125,  125,  124,  124,  123,  123,  122,  122,  
121,  121,  120,  120,  119,  119,  118,  118,  117,  117,  116,  116,  115,  115,  114,  114,  
113,  113,  113,  112,  112,  111,  111,  110,  110,  109,  109,  108,  108,  107,  107,  106,  
106,  105,  105,  104,  104,  103,  103,  103,  102,  102,  101,  101,  100,  100,  99,   99,   
98,   98,   97,   97,   97,   96,   96,   95,   95,   94,   94,   93,   93,   93,   92,   92,   
91,   91,   90,   90,   89,   89,   89,   88,   88,   87,   87,   86,   86,   86,   85,   85,   
84,   84,   83,   83,   83,   82,   82,   81,   81,   80,   80,   80,   79,   79,   78,   78,   
78,   77,   77,   76,   76,   76,   75,   75,   74,   74,   74,   73,   73,   72,   72,   72,   
71,   71,   70,   70,   70,   69,   69,   68,   68,   68,   67,   67,   66,   66,   66,   65,   
65,   65,   64,   64,   63,   63,   63,   62,   62,   62,   61,   61,   60,   60,   60,   59,   
59,   59,   58,   58,   58,   57,   57,   57,   56,   56,   55,   55,   55,   54,   54,   54,   
53,   53,   53,   52,   52,   52,   51,   51,   51,   50,   50,   50,   49,   49,   49,   48,   
48,   48,   47,   47,   47,   46,   46,   46,   45,   45,   45,   44,   44,   44,   44,   43,   
43,   43,   42,   42,   42,   41,   41,   41,   40,   40,   40,   40,   39,   39,   39,   38,   
38,   38,   37,   37,   37,   37,   36,   36,   36,   35,   35,   35,   35,   34,   34,   34,   
34,   33,   33,   33,   32,   32,   32,   32,   31,   31,   31,   31,   30,   30,   30,   29,   
29,   29,   29,   28,   28,   28,   28,   27,   27,   27,   27,   26,   26,   26,   26,   25,   
25,   25,   25,   25,   24,   24,   24,   24,   23,   23,   23,   23,   22,   22,   22,   22,   
22,   21,   21,   21,   21,   20,   20,   20,   20,   20,   19,   19,   19,   19,   19,   18,   
18,   18,   18,   18,   17,   17,   17,   17,   17,   16,   16,   16,   16,   16,   15,   15,   
15,   15,   15,   14,   14,   14,   14,   14,   14,   13,   13,   13,   13,   13,   12,   12,   
12,   12,   12,   12,   11,   11,   11,   11,   11,   11,   11,   10,   10,   10,   10,   10,   
10,   9,    9,    9,    9,    9,    9,    9,    8,    8,    8,    8,    8,    8,    8,    7,    
7,    7,    7,    7,    7,    7,    7,    6,    6,    6,    6,    6,    6,    6,    6,    6,    
5,    5,    5,    5,    5,    5,    5,    5,    5,    4,    4,    4,    4,    4,    4,    4,    
4,    4,    4,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    2,    
2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    1,    1,    
1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    
1,    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
};