function humanFileSize(bytes) {
  if (bytes === 0) {
    return "0 B";
  }

  const k = 1024;
  const sizes = ["B", "KB", "MB"];
  const i = Math.floor(Math.log(bytes) / Math.log(k));

  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i];
}

function includeHTML(id, url, callback) {
  fetch(url)
    .then((response) => response.text())
    .then((data) => {
      document.getElementById(id).innerHTML = data;
      if (typeof callback === "function") callback();
    });
}

function setHeaderTitle(title) {
  const interval = setInterval(() => {
    const h1 = document.getElementById("header-title");
    if (h1) {
      h1.textContent = title;
      clearInterval(interval);
    }
  }, 20);
}

document.addEventListener("DOMContentLoaded", () => {
  if (document.getElementById("header-placeholder")) {
    includeHTML("header-placeholder", "./header.html", () => {
      let pageTitle =
        document.title && document.title.trim()
          ? document.title.trim()
          : "Placeholder Title";
      setHeaderTitle(pageTitle);
    });
  }
  if (document.getElementById("footer-placeholder")) {
    includeHTML("footer-placeholder", "./footer.html");
  }
});
