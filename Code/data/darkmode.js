// will create a dark mode and store status in local storage

// Check if the dark mode status is stored in localStorage
var darkModeToggle = document.getElementById("darkModeToggle");

if (localStorage.getItem("darkModeStatus")) {
  // Update the dark mode toggle switch and status text
  darkModeToggle.checked = localStorage.getItem("darkModeStatus") === "On";

  // Set dark mode immediately based on stored status
  toggleDarkMode();
}

function toggleDarkMode() {
  var sliderElement = darkModeForm.querySelector(".slider");
  if (darkModeToggle.checked) {
    // Dark Mode is On
    document.documentElement.classList.add("darkmode");
    localStorage.setItem("darkModeStatus", "On");
    sliderElement.classList.remove("moon");
    sliderElement.classList.add("sun");
  } else {
    // Dark Mode is Off
    document.documentElement.classList.remove("darkmode");
    localStorage.setItem("darkModeStatus", "Off");
    sliderElement.classList.remove("sun");
    sliderElement.classList.add("moon");
  }
}

// move to admin or elsewhere afterward
document.addEventListener("DOMContentLoaded", function () {
  const topNavIcon = document.querySelector(".topnavicon");

  topNavIcon.addEventListener("click", function () {
    topNavIcon.classList.toggle("show-before");
    const afterIcon = topNavIcon.nextElementSibling;
    afterIcon.style.display = afterIcon.style.display === "none" ? "inline" : "none";
  });
});
