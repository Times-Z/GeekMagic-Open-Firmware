document.addEventListener("alpine:init", () => {
  Alpine.data("themeSwitcher", themeSwitcher);
  Alpine.data("otaUploadHandler", otaUploadHandler);
  Alpine.data("gifUploadHandler", gifUploadHandler);
  Alpine.data("wifiHandler", wifiHandler);
  Alpine.data("ntpHandler", ntpHandler);
  Alpine.data("rebootHandler", rebootHandler);
});
