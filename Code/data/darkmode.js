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
  if (darkModeToggle.checked) {
    // Dark Mode is On
    document.documentElement.classList.add("darkmode");
    localStorage.setItem("darkModeStatus", "On");
  } else {
    // Dark Mode is Off
    document.documentElement.classList.remove("darkmode");
    localStorage.setItem("darkModeStatus", "Off");
  }
}
