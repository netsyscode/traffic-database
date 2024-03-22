window.BackendURL = "http://127.0.0.1:8080"

var searchResults = document.getElementById('searchResults');
const columnTitles = ['No.', 'TimeStamp', 'Source IP', 'Destination IP', 'Source Port', 'Destination Port', 'Protocol', 'Length'];
const keys = ['number','timestamp','srcip','dstip','srcport','dstport','protocol','len']
const colorList = ['#FFD700','#FF6347','#00FFFF','#00FF00','#9370DB','#FF69B4','#00CED1','#FFA500','#7FFFD4','#9370DB']

var table = document.createElement('table');
        
var headerRow = document.createElement('tr');
columnTitles.forEach(function(title) {
    var headerCell = document.createElement('th');
    headerCell.textContent = title;
    headerRow.appendChild(headerCell);
});
table.appendChild(headerRow);
searchResults.appendChild(table);