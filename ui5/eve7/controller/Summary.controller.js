sap.ui.define([
   'sap/ui/core/mvc/Controller',
   "sap/ui/model/json/JSONModel",
   "sap/ui/core/mvc/XMLView",
   "sap/m/Button",
   "sap/m/ColorPalettePopover",
   "sap/m/StandardTreeItem",
   "sap/m/CustomTreeItem",
   "sap/m/Input",
   "sap/m/CheckBox",
   "sap/m/Panel",
   "sap/m/Text",
   "sap/ui/layout/SplitterLayoutData",
   "sap/ui/layout/VerticalLayout",
   "sap/ui/layout/HorizontalLayout"
], function(Controller, JSONModel, XMLView, Button, ColorPalettePopover,
            StandardTreeItem, CustomTreeItem,
            mInput, mCheckBox, mPanel, mText,
            SplitterLayoutData, VerticalLayout, HorizontalLayout) {

   "use strict";
   var UI5PopupColors = {
         aliceblue: 'f0f8ff',
         antiquewhite: 'faebd7',
         aqua: '00ffff',
         aquamarine: '7fffd4',
         azure: 'f0ffff',
         beige: 'f5f5dc',
         bisque: 'ffe4c4',
         black: '000000',
         blanchedalmond: 'ffebcd',
         blue: '0000ff',
         blueviolet: '8a2be2',
         brown: 'a52a2a',
         burlywood: 'deb887',
         cadetblue: '5f9ea0',
         chartreuse: '7fff00',
         chocolate: 'd2691e',
         coral: 'ff7f50',
         cornflowerblue: '6495ed',
         cornsilk: 'fff8dc',
         crimson: 'dc143c',
         cyan: '00ffff',
         darkblue: '00008b',
         darkcyan: '008b8b',
         darkgoldenrod: 'b8860b',
         darkgray: 'a9a9a9',
         darkgrey: 'a9a9a9',
         darkgreen: '006400',
         darkkhaki: 'bdb76b',
         darkmagenta: '8b008b',
         darkolivegreen: '556b2f',
         darkorange: 'ff8c00',
         darkorchid: '9932cc',
         darkred: '8b0000',
         darksalmon: 'e9967a',
         darkseagreen: '8fbc8f',
         darkslateblue: '483d8b',
         darkslategray: '2f4f4f',
         darkslategrey: '2f4f4f',
         darkturquoise: '00ced1',
         darkviolet: '9400d3',
         deeppink: 'ff1493',
         deepskyblue: '00bfff',
         dimgray: '696969',
         dimgrey: '696969',
         dodgerblue: '1e90ff',
         firebrick: 'b22222',
         floralwhite: 'fffaf0',
         forestgreen: '228b22',
         fuchsia: 'ff00ff',
         gainsboro: 'dcdcdc',
         ghostwhite: 'f8f8ff',
         gold: 'ffd700',
         goldenrod: 'daa520',
         gray: '808080',
         grey: '808080',
         green: '008000',
         greenyellow: 'adff2f',
         honeydew: 'f0fff0',
         hotpink: 'ff69b4',
         indianred: 'cd5c5c',
         indigo: '4b0082',
         ivory: 'fffff0',
         khaki: 'f0e68c',
         lavender: 'e6e6fa',
         lavenderblush: 'fff0f5',
         lawngreen: '7cfc00',
         lemonchiffon: 'fffacd',
         lightblue: 'add8e6',
         lightcoral: 'f08080',
         lightcyan: 'e0ffff',
         lightgoldenrodyellow: 'fafad2',
         lightgray: 'd3d3d3',
         lightgrey: 'd3d3d3',
         lightgreen: '90ee90',
         lightpink: 'ffb6c1',
         lightsalmon: 'ffa07a',
         lightseagreen: '20b2aa',
         lightskyblue: '87cefa',
         lightslategray: '778899',
         lightslategrey: '778899',
         lightsteelblue: 'b0c4de',
         lightyellow: 'ffffe0',
         lime: '00ff00',
         limegreen: '32cd32',
         linen: 'faf0e6',
         magenta: 'ff00ff',
         maroon: '800000',
         mediumaquamarine: '66cdaa',
         mediumblue: '0000cd',
         mediumorchid: 'ba55d3',
         mediumpurple: '9370db',
         mediumseagreen: '3cb371',
         mediumslateblue: '7b68ee',
         mediumspringgreen: '00fa9a',
         mediumturquoise: '48d1cc',
         mediumvioletred: 'c71585',
         midnightblue: '191970',
         mintcream: 'f5fffa',
         mistyrose: 'ffe4e1',
         moccasin: 'ffe4b5',
         navajowhite: 'ffdead',
         navy: '000080',
         oldlace: 'fdf5e6',
         olive: '808000',
         olivedrab: '6b8e23',
         orange: 'ffa500',
         orangered: 'ff4500',
         orchid: 'da70d6',
         palegoldenrod: 'eee8aa',
         palegreen: '98fb98',
         paleturquoise: 'afeeee',
         palevioletred: 'db7093',
         papayawhip: 'ffefd5',
         peachpuff: 'ffdab9',
         peru: 'cd853f',
         pink: 'ffc0cb',
         plum: 'dda0dd',
         powderblue: 'b0e0e6',
         purple: '800080',
         red: 'ff0000',
         rosybrown: 'bc8f8f',
         royalblue: '4169e1',
         saddlebrown: '8b4513',
         salmon: 'fa8072',
         sandybrown: 'f4a460',
         seagreen: '2e8b57',
         seashell: 'fff5ee',
         sienna: 'a0522d',
         silver: 'c0c0c0',
         skyblue: '87ceeb',
         slateblue: '6a5acd',
         slategray: '708090',
         slategrey: '708090',
         snow: 'fffafa',
         springgreen: '00ff7f',
         steelblue: '4682b4',
         tan: 'd2b48c',
         teal: '008080',
         thistle: 'd8bfd8',
         tomato: 'ff6347',
         turquoise: '40e0d0',
         violet: 'ee82ee',
         wheat: 'f5deb3',
         white: 'ffffff',
         whitesmoke: 'f5f5f5',
         yellow: 'ffff00',
         yellowgreen: '9acd32',
         transparent: '00000000'
   };// colorButton colors


   // TODO: move to separate file
   var EVEColorButton = Button.extend("rootui5.eve7.controller.EVEColorButton", {
      // when default value not specified - openui tries to load custom
      renderer: {}, // ButtonRenderer.render,

      metadata: {
         properties: {
            background: 'string'
         }
      },

      onAfterRendering: function() {
         this.$().children().css("background-color", this.getBackground());
      }

   });

   var EveSummaryTreeItem = StandardTreeItem.extend('rootui5.eve7.lib.EveSummaryTreeItem', {
      // when default value not specified - openui tries to load custom
      renderer: {},

      metadata: {
         properties: {
            background: 'string',
            mainColor: 'string'
         }
      },

      onAfterRendering: function() {
         if (this.getType() == "Active") {
            // TODO: find better way to select check box
            var chkbox = this.$().children().first().next();
            chkbox.css("display","none");
         } else {
            this.$().children().last().css("background-color", this.getMainColor());
         }

         // this.$().css("background-color", this.getBackground());
      }

   });

   return Controller.extend("rootui5.eve7.controller.Summary", {

      onInit: function () {

         var data = [{ fName: "Event" }];

         this.summaryElements = {}; // object with all elements, used for fast access to elements by id

         var oTree = this.getView().byId("tree");
         oTree.setMode(sap.m.ListMode.MultiSelect);
         oTree.setIncludeItemInSelection(true);
         this.expandLevel = 2;

         var oModel = new JSONModel();
         oModel.setData([]);
         oModel.setSizeLimit(10000);
         oModel.setDefaultBindingMode("OneWay");
         this.getView().setModel(oModel, "treeModel");

         var oItemTemplate = new EveSummaryTreeItem({
            title: "{treeModel>fName}",
            visible: "{treeModel>fVisible}",
            type: "{treeModel>fType}",
            highlight: "{treeModel>fHighlight}",
            background: "{treeModel>fBackground}",
            tooltip: "{treeModel>fTitle}",
            mainColor: "{treeModel>fMainColor}",
            selected: "{treeModel>fSelected}"
         });

/*
         var oItemTemplate = new CustomTreeItem({
            content: [
                 new mCheckBox({ selected: "{treeModel>fVisible}", select: this.clickItemSelected.bind(this) }),
                 new mText({text:" {treeModel>fName}", tooltip: "{treeModel>fTitle}" , renderWhitespace: true, wrapping: false })
            ]
         });
*/
         oItemTemplate.addStyleClass("eveSummaryItem");
         oItemTemplate.attachDetailPress({}, this.onDetailPress, this);
         oItemTemplate.attachBrowserEvent("mouseenter", this.onMouseEnter, this);
         oItemTemplate.attachBrowserEvent("mouseleave", this.onMouseLeave, this);
         oTree.bindItems("treeModel>/", oItemTemplate);
         this.template = oItemTemplate;


         this.oModelGED = new JSONModel({ title: "GED title", "widgetlist" : [] });
         this.getView().setModel(this.oModelGED, "ged");

         let make_col_obj = function(stem) {
            return { name: stem, member: "f" + stem, srv: "Set" + stem + "RGB", _type: "Color" };
         };

         let make_main_col_obj = function(label, use_main_setter) {
            return { name: label, member: "fMainColor", srv: "Set" + (use_main_setter ? "MainColor" : label) + "RGB", _type: "Color" };
         };

         this.oGuiClassDef = {
            "REveElement" : [
               { name : "RnrSelf",     _type : "Bool" },
               { name : "RnrChildren", _type : "Bool" },
               make_main_col_obj("Color", true),
               { name : "Destroy",  member : "fElementId", srv : "Destroy",  _type : "Action" },
            ],
            "REveElementList" : [ { sub: ["REveElement"] }, ],
            "REveSelection"   : [ make_col_obj("VisibleEdgeColor"), make_col_obj("HiddenEdgeColor"), ],
            "REveGeoShape"    : [ { sub: ["REveElement"] } ],
            "REveCompound"    : [ { sub: ["REveElement"] } ],
            "REvePointSet" : [
               { sub: ["REveElement" ] },
               { name : "MarkerSize", _type : "Number" }
            ],
            "REveJetCone" : [
               { name : "RnrSelf", _type : "Bool" },
               make_main_col_obj("ConeColor", true),
               { name : "NDiv",    _type : "Number" }
            ],
            "REveDataCollection" : [
               { name : "FilterExpr",  _type : "String",   quote : 1 },
               { name : "CollectionVisible",  member :"fRnrSelf",  _type : "Bool" },
               make_main_col_obj("CollectionColor")
            ],
            "REveDataItem" : [
               make_main_col_obj("ItemColor"),
               { name : "ItemRnrSelf",   member : "fRnrSelf",  _type : "Bool" },
               { name : "Filtered",   _type : "Bool" }
            ],
            "REveTrack" : [
               { name : "RnrSelf",   _type : "Bool" },
               make_main_col_obj("LineColor", true),
               { name : "LineWidth", _type : "Number" },
               { name : "Destroy",  member : "fElementId",  srv : "Destroy", _type : "Action" }
            ],
         };

         this.rebuild = false;
      },

      clickItemSelected: function(oEvent) {

      },

      onSelectionChange: function(oEvent) {

         var items = oEvent.getParameters().listItems;
         if (!items) return;

         for (var k = 0; k < items.length; ++k) {
            var item = items[k];
            if (!item) continue;

            var  path = item.getBindingContext("treeModel").getPath(),
                 ttt = item.getBindingContext("treeModel").getProperty(path);

            // console.log('path', path, 'selected', item.getSelected(), ttt.path, "is_same_path", path == ttt.path);

            var elem = this.mgr.GetElement(ttt.id);

            this.mgr.SendMIR("SetRnrSelf(" + item.getSelected() + ")", elem.fElementId, elem._typename);
         }

      },

      SetMgr: function(mgr) {
         this.mgr = mgr;
         this.mgr.RegisterController(this);
         this.selected = {}; // container of selected objects

      },

      OnEveManagerInit: function() {
         var model = this.getView().getModel("treeModel");
         model.setData(this.createSummaryModel());
         model.refresh();

         var oTree = this.getView().byId("tree");
         oTree.expandToLevel(this.expandLevel);

         // hide editor
         if (this.ged) {
            var gedFrame =  this.gedVert;
            gedFrame.unbindElement();
            gedFrame.destroyContent();
         }

         var scenes = this.mgr.childs[0].childs[2].childs;
         for (var i = 0; i < scenes.length; ++i ) {
            this.mgr.RegisterSceneReceiver(scenes[i].fElementId, this);
         }
      },

      addNodesToTreeItemModel: function(el, model) {
         // console.log("FILL el ", el.fName)
         model.fName = el.fName;
         model.guid = el.guid;
         if (el.arr) {
            model.arr = new Array(el.arr.length);
            for (var n=0; n< el.arr.length; ++n) {
               model.arr[n]= { fName: "unset"};
               this.addNodesToTreeItemModel(el.arr[n], model.arr[n]);
            }
         }

         /*
           for (var n=0; n< lst.arr.length; ++n)
           {
           var el = lst.arr[n];
           var node = {
           "fName" : el.fName,
           "guid" : el.guid
           };

           model.arr.push(node);
           if (el.arr) {
           node.arr = [];
           this.addNodesToTreeItemModel(el, node);
           }
           }
    */
      },

      addNodesToCustomModel:function(lst, model) {/*
                      for ((var n=0; n< lst.arr.length; ++n))
                      {
                      var el = lst.arr[n];
                      var node = {fName : el.fName , guid : el.guid};
                      model.push(node);
                      if (el.arr) {
                      node.arr = [];
                      addNodesToTreeItemModel(el, node);
                      }
                      }
                    */
      },

      makeDataForGED: function (element) {
         // remove ROOT::Experimental::
         var shtype = element._typename.substring(20);
         var cgd = this.oGuiClassDef[shtype];
         var arrw = [];
         var modelw = [];

         this.maxLabelLength = 0;
         var off = 0;

         // sub editors
         var subEds= [];
         if (cgd[0].sub) {
            off = 1;
            var sarr = cgd[0].sub;
            for (var i = 0; i< sarr.length; ++i) {
               var x = this.oGuiClassDef[sarr[i]];
               for (var j=0; j < x.length; j++)
               {
                  arrw.push(x[j]);
               }
            }
         }

         for (var i = off; i < cgd.length; ++i)
         {
            arrw.push(cgd[i]);
         }

          for (var i=0; i< arrw.length; ++i) {
            var parName = arrw[i].name;

            if (!arrw[i].member) {
               arrw[i].member = "f" + parName;
            }

            if (!arrw[i].srv) {
               arrw[i].srv = "Set" + parName;
            }

            var v  = element[arrw[i].member];
            if (arrw[i]._type == "Color") {
               v = JSROOT.Painter.root_colors[v];
            }
            var labeledInput = {
               value: v,
               name: arrw[i].name,
               data: arrw[i]
            };

            modelw.push({ value: v, name: arrw[i].name, data: arrw[i]});

            if (this.maxLabelLength < arrw[i].name.length) this.maxLabelLength = arrw[i].name.length;
          }

          this.oModelGED.setProperty("/widgetlist", modelw);
      },

      onItemPressed: function(oEvent) {
         var model = oEvent.getParameter("listItem").getBindingContext("treeModel"),
             path =  model.getPath(),
             ttt = model.getProperty(path);

         console.log("Summary::onItemPressed ", this.mgr.GetElement(ttt.id));
         if (!ttt || (ttt.childs !== undefined) || !ttt.id) return;
        // this.setElementSelected(ttt.id, sel_color, undefined);
      },


      onToggleOpenState: function(oEvent) {
      },

      processHighlight: function(kind, evid, force) {

         if (!force) {
            if (this._trigger_timer)
               clearTimeout(this._trigger_timer);

            this._trigger_timer = setTimeout(this.processHighlight.bind(this,kind,evid,true), 200);
            return;
         }

         delete this._trigger_timer;

         var objid = 0;

         if (kind != "leave") {
            var tree = this.getView().byId("tree"),
                items = tree.getItems(true),         item = null;
            for (var n = 0; n < items.length; ++n)
               if (items[n].getId() == evid) {
                  item = items[n]; break;
               }

            if (item) {
               var path = item.getBindingContext("treeModel").getPath();
               var ttt = item.getBindingContext("treeModel").getProperty(path);
               objid = ttt.id;
            }
         }

         // FIXME: provide more generic code which should
         this.mgr.SendMIR("NewElementPicked(" + objid + ",false,false)",
                          this.mgr.global_highlight_id, "ROOT::Experimental::REveSelection");
      },

      onMouseEnter: function(oEvent) {
         this.processHighlight("enter", oEvent.target.id);
      },

      onMouseLeave: function(oEvent) {
         this.processHighlight("leave");
      },

      GetSelectionColor:function(selection_obj) {
         return selection_obj.fName == "Global Highlight" ? "rgb(230, 230, 230)" : "rgb(66, 124, 172)";
      },

      FindTreeItemForEveElement:function(element_id) {
         var items = this.getView().byId("tree").getItems();
         for (var n = 0; n<items.length;++n) {
            var item = items[n],
                ctxt = item.getBindingContext("treeModel"),
                path = ctxt.getPath(),
                ttt = item.getBindingContext("treeModel").getProperty(path);

            if (ttt.id == element_id)
               return item;
         }
         return null;
      },

      SelectElement: function(selection_obj, element_id, sec_idcs) {
         let item = this.FindTreeItemForEveElement(element_id);
         if (item) {
            let color = this.GetSelectionColor(selection_obj);
            item.$().css("background-color", color);
         }
      },

      UnselectElement: function (selection_obj, element_id) {
         let item = this.FindTreeItemForEveElement(element_id);
         if (item) {
            let color = this.GetSelectionColor(selection_obj);
            let cc = item.$().css("background-color");
            if (cc == color)
               item.$().css("background-color", "");
         }
      },

      /** When edit button pressed */
      onDetailPress: function(oEvent) {
         var item = oEvent.getSource(),
             path = item.getBindingContext("treeModel").getPath(),
             ttt = item.getBindingContext("treeModel").getProperty(path);

         this.showGedEditor(path, ttt);
      },

      showGedEditor: function(path, newelem) {

         var sumSplitter = this.byId("sumSplitter");

         if (!this.ged) {
            var pthis = this;

            XMLView.create({
               viewName: "rootui5.eve7.view.Ged",
               viewData: { summaryCtrl : this },
               layoutData: new SplitterLayoutData("sld", {size : "30%"}),
               height: "100%"
            }).then(function(oView) {
               sumSplitter.addContentArea(oView);

               oView.setModel(pthis.oModelGED,"ged");


               pthis.ged = oView;
               pthis.gedVert = oView.byId("GED");
               pthis.ged_visible = true;
               pthis.fillGedEditor(path, newelem);

            });
         } else if (this.ged_visible) {
            if (this.ged_path == path) {
               this.ged_visible = false;
               sumSplitter.removeContentArea(this.ged);
            } else {
               this.ged_path = path;
               this.fillGedEditor(path, newelem);
            }
         } else {
            this.ged_visible = true;
            sumSplitter.addContentArea(this.ged);
            this.fillGedEditor(path, newelem);
         }
      },

      closeGedEditor: function() {
         if (this.ged && this.ged_visible) {
            this.ged_visible = false;
            this.ged_path = "";
            this.byId("sumSplitter").removeContentArea(this.ged);
         }
      },

      fillGedEditor: function(itemPath, newelem) {
         this.ged_path = itemPath;

         this.editorElement = this.mgr.GetElement(newelem.id);

         var oProductDetailPanel = this.ged;
        // var oProductDetailPanel = this.byId("productDetailsPanel");

         var title = this.editorElement.fName + " (" +  this.editorElement._typename.substring(20) + " )" ;
         // oProductDetailPanel.setHeaderText(title);

         this.oModelGED.setProperty("/title", title);

         oProductDetailPanel.bindElement({ path: itemPath, model: "event" });

         var gedFrame =  this.gedVert;
         gedFrame.unbindElement();
         gedFrame.destroyContent();

         this.makeDataForGED(this.editorElement);

         // console.log("going to bind >>> ", this.getView().getModel("ged"));
         gedFrame.bindAggregation("content", "ged>/widgetlist",  this.gedFactory.bind(this) );
      },


      gedFactory: function(sId, oContext) {
         var base = "/widgetlist/";
         var path = oContext.getPath();
         var idx = path.substring(base.length);
         var customData =  oContext.oModel.oData["widgetlist"][idx].data;
         var controller = this;
         var widget = null;

         switch (customData._type) {

         case "Number":
            widget = new mInput(sId, {
               value: { path: "ged>value" },
               change: this.sendMethodInvocationRequest.bind(this, "Number")
            });
            widget.setType(sap.m.InputType.Number);
            break;

         case "String":
            widget = new mInput(sId, {
               value: { path: "ged>value" },
               change: this.sendMethodInvocationRequest.bind(this, "String")

            });
            widget.setType(sap.m.InputType.String);
            widget.setWidth("250px"); // AMT this should be handled differently
            break;
         case "Bool":
            widget = new mCheckBox(sId, {
               selected: { path: "ged>value" },
               select: this.sendMethodInvocationRequest.bind(this, "Bool")
            });
            break;

         case "Color":
            var colVal = oContext.oModel.oData["widgetlist"][idx].value;
            // var model = this.getView().getModel("colors");
            //   model["mainColor"] = colVal;
            //  console.log("col value ", colVal, JSROOT.Painter.root_colors[colVal]);
            widget = new EVEColorButton(sId, {
               icon: "sap-icon://palette",
               background: colVal,

               press: function () {
                  var colButton = this;
                  var oCPPop = new ColorPalettePopover( {
                      defaultColor: "cyan",
                       colors: ['gold','darkorange', 'indianred','rgb(102,51,0)', 'cyan',// 'magenta'
                                'blue', 'lime', 'gray','slategray','rgb(204, 198, 170)',
                                'white', 'black','red' , 'rgb(102,154,51)', 'rgb(200, 0, 200)'],
                       colorSelect: function(event) {
                          colButton.setBackground(event.getParameters().value);
                          controller.handleColorSelect(event);
                       }
                   });

                   oCPPop.openBy(colButton);
                   oCPPop.data("myData", customData);
                 }
            });
            break;

         case "Action":
            widget = new Button(sId, {
               //text: "Action",
               icon: "sap-icon://accept",
               press: this.sendMethodInvocationRequest.bind(this, "Action")
            });
            break;
         }

         if (widget) widget.data("myData", customData);

         var label = new mText(sId + "label", { text: { path: "ged>name" } });
         label.setWidth(this.maxLabelLength +"ex");
         label.addStyleClass("sapUiTinyMargin");

         return new HorizontalLayout({
            content : [label, widget]
         });
      },

      handleColorSelect: function(event) {
          var val = event.getParameters().value;
          var myData = event.getSource().data("myData");

         var rgb,
             regex = /rgb\((\d+)\,\s?(\d+)\,\s?(\d+)\)/,
             found = val.match(regex);
         if (found) {
            console.log("match color ", found);
            /*
            rgb.r = found[1];
            rgb.g = found[2];
            rgb.b = found[3];
            */
            rgb = { r: found[1], g: found[2], b: found[3] };
         } else {
            var hex = UI5PopupColors[val];

            // Expand shorthand form (e.g. "03F") to full form (e.g. "0033FF")
            var shorthandRegex = /^#?([a-f\d])([a-f\d])([a-f\d])$/i;

            hex = hex.replace(shorthandRegex, function(m, r, g, b) {
               return r + r + g + g + b + b;
            });

            rgb = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);

            rgb = rgb ? { r: parseInt(rgb[1], 16), g: parseInt(rgb[2], 16), b: parseInt(rgb[3], 16) } : null;
         }

         var mir =  myData.srv + "((UChar_t)" + rgb.r + ", (UChar_t)" + rgb.g +  ", (UChar_t)" + rgb.b + ")";
         this.mgr.SendMIR(mir, this.editorElement.fElementId, this.editorElement._typename);
      },

      sendMethodInvocationRequest: function(kind, event) {
         var value = "";
         switch (kind) {
            case "Bool": value = event.getSource().getSelected(); break;
            case "Action": value = ""; break;
            default: value =  event.getParameter("value");
         }

         var myData = event.getSource().data("myData");

         if (myData.quote !== undefined)
              value = "\"" + value + " \"";

         this.mgr.SendMIR(myData.srv + "( " + value + " )", this.editorElement.fElementId, this.editorElement._typename );
      },

      changeNumPoints:function() {
         var myJSON = "changeNumPoints(" +  this.editorElement.guid + ", "  + this.editorElement.fN +  ")";
         this.mgr.handle.Send(myJSON);
      },

      printEvent: function(event) {
         var propertyPath = event.getSource().getBinding("value").getPath();
         // console.log("property path ", propertyPath);
         var bindingContext = event.getSource().getBindingContext("event");

         var path =  bindingContext.getPath(propertyPath);
         var object =  bindingContext.getObject(propertyPath);
         // console.log("obj ",object );

         this.changeNumPoints();
      },

      changeRnrSelf: function(event) {
         var myJSON = "changeRnrSelf(" +  this.editorElement.guid + ", "  + event.getParameters().selected +  ")";
         this.mgr.handle.Send(myJSON);
      },

      changeRnrChld: function(event) {
         console.log("change Rnr ", event, " source ", event.getSource());
      },

      updateGED : function (elementId) {
         if (!this.editorElement) return;
         if (this.editorElement.fElementId == elementId) {
            var gedFrame =  this.gedVert;
            gedFrame.unbindElement();
            gedFrame.destroyContent();
            this.makeDataForGED(this.editorElement);
            gedFrame.bindAggregation("content", "ged>/widgetlist", this.gedFactory.bind(this));
         }
      },

      canEdit: function(elem) {
         var t = elem._typename.substring(20);
         var ledit = this.oGuiClassDef;
         if (ledit.hasOwnProperty(t))
            return true;
         return false;
      },

      anyVisible: function(arr) {
         if (!arr) return false;
         for (var k=0;k<arr.length;++k) {
            if (arr[k].fName) return true;
         }
         return false;
      },

      /** Set summary element attributes from original element */
      setElementsAttributes: function(newelem, elem) {
         if (this.canEdit(elem)) {
            newelem.fType = "DetailAndActive";
            if (elem.fMainColor) {
               newelem.fMainColor = JSROOT.Painter.root_colors[elem.fMainColor];
               newelem.fSelected = elem.fRnrSelf;
            }
         } else {
            newelem.fType = "Active";
         }
      },

      createSummaryModel: function(tgt, src, path) {
         if (tgt === undefined) {
            tgt = [];
            src = this.mgr.childs;
            this.summaryElements = {};
            path = "/";
            // console.log('original model', src);
         }
         for (var n=0;n<src.length;++n) {
            var elem = src[n];

            var newelem = { fName: elem.fName, fTitle: elem.fTitle || elem.fName, id: elem.fElementId, fHighlight: "None", fBackground: "", fMainColor: "", fSelected: false };

            this.setElementsAttributes(newelem, elem);

            newelem.path = path + n;
            newelem.masterid = elem.fMasterId || elem.fElementId;

            tgt.push(newelem);

            this.summaryElements[newelem.id] = newelem;

            if ((elem.childs !== undefined) && this.anyVisible(elem.childs))
               newelem.childs = this.createSummaryModel([], elem.childs, newelem.path + "/childs/");
         }

         return tgt;
      },

      beginChanges: function() {
        // this.rebuild=false;
      },

      elementsRemoved: function(ids) {
         this.rebuild = true;
      },

      sceneElementChange: function(msg) {

         if (this.editorElement && (this.editorElement.fElementId == msg.fElementId))
            this.updateGED(msg.fElementId);

         var newelem = this.summaryElements[msg.fElementId];

         var elem = this.mgr.GetElement(msg.fElementId);

         if (newelem && elem)
            this.setElementsAttributes(newelem, elem);

         // console.log('SUMMURY: detect changed', elem.id, elem.path);

         if (msg.rnr_self_changed)
            elem.fSelected = msg.fRnrSelf;

         this.any_changed = true;

      },

      endChanges: function() {
         if (this.rebuild) {
            var oTree = this.getView().byId("tree");
            oTree.unbindItems();

            var model = this.getView().getModel("treeModel");
            model.setData(this.createSummaryModel());
            model.refresh();

            this.getView().setModel(model, "treeModel");
            oTree.bindItems("treeModel>/", this.template);
            oTree.setModel(model, "treeModel");

            oTree.expandToLevel(this.expandLevel);

            if (this.ged && this.ged.visible) {
               var pp = this.byId("sumSplitter");
               pp.removeContentArea(this.ged);
               this.ged.visible = false;
            }

            this.rebuild = false;
         } else if (this.any_changed) {
            var model = this.getView().getModel("treeModel");
            model.refresh();

            this.any_changed = false;
         }
      }
   });
});
