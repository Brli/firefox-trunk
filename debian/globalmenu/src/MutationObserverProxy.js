const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function MutationRecord(aRecord)
{
  this.type = aRecord.type;
  this.target = aRecord.target;
  this.addedNodes = aRecord.addedNodes;
  this.removedNodes = aRecord.removedNodes;
  this.previousSibling = aRecord.previousSibling;
  this.attributeName = aRecord.attributeName;
}

MutationRecord.prototype = {
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.uIGlobalMenuMutationRecord]
  )
};

function MutationObserverProxy() {}

MutationObserverProxy.prototype = {
  classDescription: "DOM Mutation Observer Proxy for Ubuntu Global Menu",
  classID: Components.ID("{2084d756-7c14-4aec-8238-93e2b17a581d}"),
  contractID: "@canonical.com/globalmenu-mutation-observer-proxy;1",

  QueryInterface: XPCOMUtils.generateQI(
    [Ci.uIGlobalMenuMutationObserverProxy]
  ),

  init: function(aDoc, aListener) {
    if (this.obs) {
      throw Components.Exception("Called init more than once",
                                 Cr.NS_ERROR_FAILURE);
    }

    if (!(aListener instanceof Ci.uIGlobalMenuMutationObserver)) {
      throw Components.Exception("Invalid listener", Cr.NS_ERROR_FAILURE);
    }

    this.obs = new aDoc.defaultView.MutationObserver(function(mutations) {
      let records = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
      mutations.forEach(function(mutation) {
        records.appendElement(new MutationRecord(mutation), false);
      });

      aListener.handleMutations(records);
    });

    this.obs.observe(aDoc, {childList: true, attributes: true, subtree: true}); 
  },

  disconnect: function() {
    if (!this.obs) {
      throw Components.Exception("Called disconnect before init",
                                 Cr.NS_ERROR_FAILURE);
    }

    this.obs.disconnect();
    this.obs = null;
  }
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([MutationObserverProxy]);
