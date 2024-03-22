document.getElementById('searchButton').addEventListener('click', function() {
    // 获取搜索框内的文字
    var searchTerm = document.getElementById('searchInput').value;
    var formData = new FormData();
    formData.append('searchKey', searchTerm);

    // 发送搜索请求到后端接口
    fetch(window.BackendURL+'/search', {
        method: 'POST',
        // headers: {
        //     'Content-Type': 'multipart/form-data'
        // },
        body: formData
    })
    .then(response => response.json()) // 解析后端返回的 JSON 数据
    .then(data => {
        // 清空搜索结果表格
        // var searchResults = document.getElementById('searchResults');
        searchResults.innerHTML = '';

        // 创建搜索结果表格
        var table = document.createElement('table');       
        // 创建表头
        var headerRow = document.createElement('tr');
        columnTitles.forEach(function(title) {
            var headerCell = document.createElement('th');
            headerCell.textContent = title;
            headerRow.appendChild(headerCell);
        });
        table.appendChild(headerRow);

        data.data.forEach(function(result) {
            var row = document.createElement('tr');

            row.style.backgroundColor = colorList[result['hash']%colorList.length]

            row.addEventListener('click', function() {
                // 移除其他行的选中样式
                var rows = document.querySelectorAll('#searchResults tr');
                rows.forEach(function(row) {
                    row.classList.remove('selected');
                });

                // 为被点击行添加选中样式
                row.classList.add('selected');
            });

            keys.forEach(function(title) {
                var cell = document.createElement('td');
                cell.textContent = result[title.toLowerCase()];
                row.appendChild(cell);
            });
            table.appendChild(row);
        });
        searchResults.appendChild(table);
    })
    .catch(error => {
        console.error('Error:', error);
    });
});
