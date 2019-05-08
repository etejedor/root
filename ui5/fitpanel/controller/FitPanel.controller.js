sap.ui.define([
   'rootui5/panel/Controller',
   'sap/ui/model/json/JSONModel',
   'sap/m/Button',
   'sap/m/Table',
   'sap/m/Dialog',
   'sap/m/List',
   'sap/m/Input',
   'sap/m/Label',
   'sap/m/CheckBox',
   'sap/m/Column',
   'sap/m/ColumnListItem',
   'sap/m/ColorPalettePopover'
], function (GuiPanelController, JSONModel, mButton, mTable,
             mDialog, mList, mInput, mLabel, mCheckBox, mColumn, mColumnListItem, ColorPalettePopover) {

   "use strict";

   var count = 0;
   var colorConf = "rgb(0,0,0)";

   return GuiPanelController.extend("rootui5.fitpanel.controller.FitPanel", {

         //function called from GuiPanelController
      onPanelInit : function() {

         JSROOT.loadScript('rootui5sys/fitpanel/style/style.css');

         var id = this.getView().getId();
         this.inputId = "";
         var opText = this.getView().byId("OperationText");
         var data = {
               //fDataSet:[ { fId:"1", fSet: "----" } ],
               fSelectDataId: "2",
               // fMinRange: -4,
               // fMaxRange: 4,
               fStep: 0.01,
               fRange: [-4,4],
               fUpdateRange: [-4,4]
         };
         this.getView().setModel(new JSONModel(data));
         this._data = data; // current data object
         this.copyModel = []; // array to keep version on the model
         this.modelCount = 0;
      },

      // Assign the new JSONModel to data
      OnWebsocketMsg: function(handle, msg){

         if(msg.startsWith("MODEL:")){
            var data = JSROOT.parse(msg.substr(6));
            if(data) {
               data.fFuncList = data.fFuncListAll[parseInt(data.fSelectTypeFunc)];
               data.fMethodMin = data.fMethodMinAll[parseInt(data.fLibrary)];

               this.getView().setModel(new JSONModel(data));
               this._data = data;

               this.copyModel = [ JSROOT.extend({}, data) ];
               this.modelCount = 0;
            }
         } else if (msg.startsWith("PARS:")) {

            var data = this.getView().getModel().getData();

            data.fFuncPars = JSROOT.parse(msg.substr(5));

            this.getView().getModel().refresh();

         } else if (msg.startsWith("ADVANCED:")) {
         	var data = JSROOT.parse(msg.substr(9));
         	//this.getAdvanced();
         	if(data) {
               this.getView().setModel(new JSONModel(data));
               this._data = data;
            }
         }

      },

      //Fitting Button
      doFit: function() {
         //Keep the #times the button is clicked
         //Data is a new model. With getValue() we select the value of the parameter specified from id
         var data = this.getView().getModel().getData();

         var libMin = this.getView().byId("MethodMin").getValue();
         data.fMinLibrary = libMin;

         var errorDefinition = parseFloat(this.getView().byId("errorDef").getValue());
         data.fErrorDef = errorDefinition;
         var maxTolerance = parseFloat(this.getView().byId("maxTolerance").getValue());
         data.fMaxTol = maxTolerance;
         var maxInterations = Number(this.getView().byId("maxInterations").getValue());
         data.fMaxInter = maxInterations;

         //Refresh the model
         this.getView().getModel().refresh();
         //Each time we click the button, we keep the current state of the model
         this.copyModel[++this.modelCount] = JSROOT.extend({},data);
         //console.log("DOFIT " + this.getView().getModel().getJSON());

         // TODO: skip "fMethodMin" and "fFuncList" from output object
         // Requires changes in JSROOT.toJSON(), can be done after REVE-selection commit

         if (this.websocket)
            this.websocket.Send('DOFIT:'+this.getView().getModel().getJSON());

      },

      onPanelExit: function(){

      },

      resetPanel: function(oEvent){

         if(!this.copyModel[0]) return;

         JSROOT.extend(this._data, this.copyModel[0]);

         this.getView().getModel().updateBindings();

         this.byId("selectedOpText").setText("gaus");
         this.byId("OperationText").setValue("");
         this.byId("errorDef").setValue("");
         this.gbyId("maxTolerance").setValue("");
         this.byId("maxInterations").setValue("");
      },

      backPanel: function() {
         //Each time we click the button, we go one step back
         if (this.modelCount <= 0) return;
         this.modelCount--;

         if(!this.copyModel[this.modelCount]) return;

         JSROOT.extend(this._data, this.copyModel[this.modelCount]);
         this.getView().getModel().updateBindings();
      },

      /** Returns selected function name */
      getSelectedFunc: function() {
         var data = this.getView().getModel().getData();
         return data.fSelectedFunc;
      },

      //Change the input text field. When a function is seleced, it appears on the text input field and
      //on the text area.
      onSelectedFuncChange: function(){

         var func = this.getSelectedFunc();

         var data = this.getView().getModel().getData();
         data.fFuncChange = func; // FIXME: seems to be, not required
         this.getView().getModel().refresh();

         if (this.websocket && func)
            this.websocket.Send("GETPARS:" + func);

         //updates the text area and text in selected tab, depending on the choice in TypeXY ComboBox
         this.byId("OperationText").setValueLiveUpdate();
         this.byId("OperationText").setValue(func);
         this.byId("selectedOpText").setText(func);
      },

      operationTextChange: function(oEvent) {
         var newValue = oEvent.getParameter("value");
         this.byId("selectedOpText").setText(newValue);
      },


      //change the combo box in Minimization Tab --- Method depending on Radio Buttons values
      selectRB: function(){

         var data = this.getView().getModel().getData();

         // same code as initialization
         data.fMethodMin = data.fMethodMinAll[parseInt(data.fLibrary)];

         // refresh all UI elements
         this.getView().getModel().refresh();
      },

      //Change the combobox in Type Function
      //When the Type (TypeFunc) is changed (Predef etc) then the combobox with the funtions (TypeXY),
      //is also changed
      selectTypeFunc: function(){

         var data = this.getView().getModel().getData();

         // console.log("Func Type = " + data.fSelectTypeFunc);

         data.fFuncList = data.fFuncListAll[parseInt(data.fSelectTypeFunc)];

         this.getView().getModel().refresh();
      },

      //Change the selected checkbox of Draw Options
      //if Do not Store is selected then No Drawing is also selected
      storeChange: function(){
         var data = this.getView().getModel().getData();
         var fDraw = this.getView().byId("noStore").getSelected();
         console.log("fDraw = ", fDraw);
         data.fNoStore = fDraw;
         this.getView().getModel().refresh();
         console.log("fNoDrawing ", data.fNoStore);
      },

      drawContour: function() {

      	var data = this.getView().getModel().getData();
      	var contourPoints = this.byId("contourPoints").getValue();
      	data.fContourPoints = contourPoints;
      	var contourPar1 = parseInt(this.byId("ContourPar1").getSelectedKey());
      	data.fContourPar1 = contourPar1;
      	var contourPar2 = parseInt(this.byId("ContourPar2").getSelectedKey());
      	data.fContourPar2 = contourPar2;
      	var confLevel = this.byId("ConfLevel").getValue();
         var colorContourNum = (String((this.colorContour.replace( /^\D+/g, '')).replace(/[()]/g, ''))).split(',');
         data.fColorContour = colorContourNum;

         console.log("COLOR ", colorContourNum, typeof colorContourNum, " origin ", this.colorContour);
       //   var colConfN = colorConf.replace( /^\D+/g, '');
       //   var colorConfNum = colConfN.replace(/[()]/g, '');
      	// data.fConfLevel = colorConfNum;

	  	  this.getView().getModel().refresh();
        //Each time we click the button, we keep the current state of the model
        if (this.websocket)
            this.websocket.Send('SETCONTOUR:'+this.getView().getModel().getJSON());


      },

      drawScan: function() {

      	var data = this.getView().getModel().getData();
      	var scanPoints = this.byId("scanPoints").getValue();
      	data.fScanPoints = scanPoints;
      	var scanPar = parseInt(this.byId("ScanPar").getSelectedKey());
      	data.fScanPar = scanPar;
      	var scanMin = this.byId("scanMin").getValue();
      	data.fScanMin = scanMin;
      	var scanMax = this.byId("scanMax").getValue();
      	data.fScanMax = scanMax;


      	this.getView().getModel().refresh();
        //Each time we click the button, we keep the current state of the model
        if (this.websocket)
            this.websocket.Send('SETSCAN:'+this.getView().getModel().getJSON());

      },

      pressApplyPars: function() {
         var data = this.getView().getModel().getData();
         var json = JSROOT.toJSON(data.fFuncPars);

         if (this.websocket)
            this.websocket.Send("SETPARS:" + json);
      },


      //Cancel Button on Set Parameters Dialog Box
      onCancel: function(oEvent){
         oEvent.getSource().close();
      },

      updateRange: function() {
         var data = this.getView().getModel().getData();
         var range = this.getView().byId("Slider").getRange();
         console.log("Slider " + range);

         //We pass the values from range array in JS to C++ fRange array
         data.fUpdateRange[0] = range[0];
         data.fUpdateRange[1] = range[1];
      },

      colorPickerContour: function (oEvent) {
         this.inputId = oEvent.getSource().getId();
         if (!this.oColorPickerPopoverContour) {
            this.oColorPickerPopoverContour = new sap.ui.unified.ColorPickerPopover({
               colorString: "blue",
               mode: sap.ui.unified.ColorPickerMode.HSL,
               change: this.handleChangeContour.bind(this)
            });
         }
         this.oColorPickerPopoverContour.openBy(oEvent.getSource());
         // if(!this.oColorPalettePopoverFull) {
         // 	this.oColorPalettePopoverFull = new ColorPalettePopover("oColorPalettePopoverFull", {
         // 		color:"blue",
         // 		colorSelect: this.handleChangeContour.bind(this)
         // 	});
         // }

         // this.oColorPalettePopoverFull.openBy(oEvent.getSource());
      },


      handleChangeContour: function (oEvent) {
         var oView = this.getView();
         this.inputId = "";
         var color1 = oEvent.getParameter("colorString");
         var oButtonContour = this.getView().byId("colorContour");
         var oButtonInnerContour = oButtonContour.$().find('.sapMBtnInner');
         oButtonInnerContour.css('background',color1);
         oButtonInnerContour.css('color','#FFFFFF');
         oButtonInnerContour.css('text-shadow','1px 1px 2px #333333');

         this.colorContour = color1;
         return this.colorContour;
	  },

	  colorPickerConf: function (oEvent) {
         this.inputId = oEvent.getSource().getId();
         if (!this.oColorPickerPopoverConf) {
            this.oColorPickerPopoverConf = new sap.ui.unified.ColorPickerPopover({
               colorString: "blue",
               mode: sap.ui.unified.ColorPickerMode.HSL,
               change: this.handleChangeConf.bind(this)
            });
         }
         this.oColorPickerPopoverConf.openBy(oEvent.getSource());
      },


      handleChangeConf: function (oEvent) {
         var oView = this.getView();
         this.inputId = "";
         var color2 = oEvent.getParameter("colorString");
         var oButtonContour = this.getView().byId("colorConf");
         var oButtonInnerContour = oButtonContour.$().find('.sapMBtnInner');
         oButtonInnerContour.css('background',color2);
         oButtonInnerContour.css('color','#FFFFFF');
         oButtonInnerContour.css('text-shadow','1px 1px 2px #333333');

         colorConf = color2;
         return colorConf;
	  }

   });

   return
});
