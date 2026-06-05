{
	"patcher" : {
		"fileversion" : 1,
		"appversion" : {
			"major" : 9,
			"minor" : 0,
			"revision" : 0,
			"architecture" : "x64",
			"modernui" : 1
		},
		"rect" : [ 100.0, 100.0, 800.0, 600.0 ],
		"boxes" : [
			{
				"box" : {
					"id" : "obj-1",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 20.0, 500.0, 20.0 ],
					"text" : "soundtypes~ -- real-time concatenative synthesis"
				}
			},
			{
				"box" : {
					"id" : "obj-2",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 42.0, 600.0, 20.0 ],
					"text" : "Analyse a corpus buffer, then match live audio to its segments in real time."
				}
			},
			{
				"box" : {
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 80.0, 400.0, 20.0 ],
					"text" : "STEP 1 -- load a sound file into the corpus buffer"
				}
			},
			{
				"box" : {
					"id" : "obj-4",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 20.0, 104.0, 35.0, 22.0 ],
					"text" : "read"
				}
			},
			{
				"box" : {
					"id" : "obj-5",
					"maxclass" : "newobj",
					"numinlets" : 0,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 20.0, 134.0, 100.0, 22.0 ],
					"text" : "buffer~ corpus"
				}
			},
			{
				"box" : {
					"id" : "obj-6",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 175.0, 400.0, 20.0 ],
					"text" : "STEP 2 -- set parameters and analyse"
				}
			},
			{
				"box" : {
					"id" : "obj-7",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 20.0, 200.0, 80.0, 22.0 ],
					"text" : "set corpus"
				}
			},
			{
				"box" : {
					"id" : "obj-8",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 110.0, 200.0, 70.0, 22.0 ],
					"text" : "clusters 5"
				}
			},
			{
				"box" : {
					"id" : "obj-9",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 190.0, 200.0, 110.0, 22.0 ],
					"text" : "sensitivity 0.25"
				}
			},
			{
				"box" : {
					"id" : "obj-10",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 310.0, 200.0, 100.0, 22.0 ],
					"text" : "minlength 200"
				}
			},
			{
				"box" : {
					"id" : "obj-11",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 420.0, 200.0, 110.0, 22.0 ],
					"text" : "threshold 0.01"
				}
			},
			{
				"box" : {
					"id" : "obj-12",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 540.0, 200.0, 20.0, 20.0 ]
				}
			},
			{
				"box" : {
					"id" : "obj-13",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 20.0, 255.0, 100.0, 22.0 ],
					"text" : "soundtypes~"
				}
			},
			{
				"box" : {
					"id" : "obj-14",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 130.0, 255.0, 300.0, 20.0 ],
					"text" : "<-- connect adc~ 1 here for live mic input"
				}
			},
			{
				"box" : {
					"id" : "obj-15",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 20.0, 295.0, 100.0, 22.0 ],
					"text" : "route segment"
				}
			},
			{
				"box" : {
					"id" : "obj-16",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 2,
					"outlettype" : [ "int", "int" ],
					"patching_rect" : [ 20.0, 325.0, 70.0, 22.0 ],
					"text" : "unpack 0 0"
				}
			},
			{
				"box" : {
					"id" : "obj-17",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "float" ],
					"patching_rect" : [ 20.0, 355.0, 50.0, 22.0 ],
					"text" : "/ 44.1"
				}
			},
			{
				"box" : {
					"id" : "obj-18",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "float" ],
					"patching_rect" : [ 90.0, 355.0, 50.0, 22.0 ],
					"text" : "/ 44.1"
				}
			},
			{
				"box" : {
					"id" : "obj-19",
					"maxclass" : "newobj",
					"numinlets" : 4,
					"numoutlets" : 3,
					"outlettype" : [ "signal", "signal", "bang" ],
					"patching_rect" : [ 20.0, 410.0, 130.0, 22.0 ],
					"text" : "groove~ corpus 0"
				}
			},
			{
				"box" : {
					"id" : "obj-20",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 320.0, 295.0, 100.0, 22.0 ],
					"text" : "route match"
				}
			},
			{
				"box" : {
					"id" : "obj-21",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 320.0, 325.0, 120.0, 22.0 ],
					"text" : "print match_info"
				}
			},
			{
				"box" : {
					"id" : "obj-22",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 470.0, 60.0, 22.0 ],
					"text" : "ezdac~"
				}
			},
			{
				"box" : {
					"id" : "obj-23",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 510.0, 650.0, 20.0 ],
					"text" : "Left outlet: segment start end (samples) | Right outlet: match index cluster distance"
				}
			},
			{
				"box" : {
					"id" : "obj-24",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 170.0, 410.0, 35.0, 22.0 ],
					"text" : "loop 1"
				}
			}
		],
		"lines" : [
			{ "patchline" : { "source" : [ "obj-4", 0 ], "destination" : [ "obj-5", 0 ] } },
			{ "patchline" : { "source" : [ "obj-7", 0 ], "destination" : [ "obj-13", 0 ] } },
			{ "patchline" : { "source" : [ "obj-8", 0 ], "destination" : [ "obj-13", 0 ] } },
			{ "patchline" : { "source" : [ "obj-9", 0 ], "destination" : [ "obj-13", 0 ] } },
			{ "patchline" : { "source" : [ "obj-10", 0 ], "destination" : [ "obj-13", 0 ] } },
			{ "patchline" : { "source" : [ "obj-11", 0 ], "destination" : [ "obj-13", 0 ] } },
			{ "patchline" : { "source" : [ "obj-12", 0 ], "destination" : [ "obj-13", 0 ] } },
			{ "patchline" : { "source" : [ "obj-13", 0 ], "destination" : [ "obj-15", 0 ] } },
			{ "patchline" : { "source" : [ "obj-13", 1 ], "destination" : [ "obj-20", 0 ] } },
			{ "patchline" : { "source" : [ "obj-15", 0 ], "destination" : [ "obj-16", 0 ] } },
			{ "patchline" : { "source" : [ "obj-16", 0 ], "destination" : [ "obj-17", 0 ] } },
			{ "patchline" : { "source" : [ "obj-16", 1 ], "destination" : [ "obj-18", 0 ] } },
			{ "patchline" : { "source" : [ "obj-17", 0 ], "destination" : [ "obj-19", 1 ] } },
			{ "patchline" : { "source" : [ "obj-18", 0 ], "destination" : [ "obj-19", 2 ] } },
			{ "patchline" : { "source" : [ "obj-19", 0 ], "destination" : [ "obj-22", 0 ] } },
			{ "patchline" : { "source" : [ "obj-20", 0 ], "destination" : [ "obj-21", 0 ] } },
			{ "patchline" : { "source" : [ "obj-24", 0 ], "destination" : [ "obj-19", 0 ] } }
		]
	}
}
