// clean-unreal.js
const fs = require("fs");
const path = require("path");

const projectRoot = process.cwd(); // run script from project root
const safeToDelete = ["Binaries", "Intermediate", "Saved", "DerivedDataCache"];

function deleteFolderRecursive(folderPath) {
  if (fs.existsSync(folderPath)) {
    fs.rmSync(folderPath, { recursive: true, force: true });
    console.log(`Deleted: ${folderPath}`);
  }
}

function traverseAndClean(dir) {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);

    if (entry.isDirectory()) {
      if (safeToDelete.includes(entry.name)) {
        deleteFolderRecursive(fullPath);
      } else {
        traverseAndClean(fullPath);
      }
    }
  }
}

console.log("Cleaning Unreal project...");
traverseAndClean(projectRoot);
console.log("Done.");
