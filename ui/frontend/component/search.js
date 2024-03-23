document.getElementById('searchButton').addEventListener('click', function() {
    // 获取搜索框内的文字
    var searchTerm = document.getElementById('searchInput').value;
    var formData = new FormData();
    formData.append('searchKey', searchTerm);

    // 发送搜索请求到后端接口
    fetch(window.BackendURL+'/search', {
        method: 'POST',
        body: formData
    })
    .then(response => response.json()) // 解析后端返回的 JSON 数据
    .then(data => {
        // 清空搜索结果表格
        // var searchResults = document.getElementById('searchResults');
        searchResults.innerHTML = '';
        metaArea.innerHTML = '';
        dataArea.innerHTML = '';

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

            keys.forEach(function(title) {
                var cell = document.createElement('td');
                cell.textContent = result[title.toLowerCase()];
                row.appendChild(cell);
            });

            row.addEventListener('click', function() {
                // 移除其他行的选中样式
                var rows = document.querySelectorAll('#searchResults tr');
                rows.forEach(function(row) {
                    row.classList.remove('selected');
                });

                // 为被点击行添加选中样式
                row.classList.add('selected');

                var resultId = result.number;

                var detailData = new FormData();
                detailData.append('packetNumber', resultId);

                console.log(resultId)

                fetch(window.BackendURL + '/packet',{
                    method: 'POST',
                    body: detailData
                })
                .then(response => response.json())
                .then(details => {
                    // 将详细信息显示在页面上
                    
                    var detailHTML = ``;
                    
                    if('l2' in details.meta){
                        detailHTML += `<p>L2: `
                        if('srcmac' in details.meta.l2 && 'dstmac' in details.meta.l2){
                            detailHTML += `${details.meta.l2.srcmac} -> ${details.meta.l2.dstmac}`
                        }
                        detailHTML += `<\p>`
                    }
                    if('l3' in details.meta){
                        detailHTML += `<p>L3: `
                        if('srcip' in details.meta.l3 && 'dstip' in details.meta.l3){
                            detailHTML += `${details.meta.l3.srcip} -> ${details.meta.l3.dstip}`
                        }
                        if('version' in details.meta.l3){
                            detailHTML += ` version: ${details.meta.l3.version}`
                        }
                        detailHTML += `<\p>`
                    }
                    if('l4' in details.meta){
                        detailHTML += `<p>L4: `
                        if('protocol' in details.meta.l4){
                            detailHTML += `${details.meta.l4.protocol} `
                        }
                        if('srcport' in details.meta.l4 && 'dstport' in details.meta.l4){
                            detailHTML += `${details.meta.l4.srcport} -> ${details.meta.l4.dstport}`
                        }
                        if('seq' in details.meta.l4){
                            detailHTML += ` sequence: ${details.meta.l4.seq} `
                        }
                        if('flags' in details.meta.l4){
                            detailHTML += ` flags: ${details.meta.l4.flags.toString(2)} `
                        }
                        if('type' in details.meta.l4){
                            detailHTML += ` type: ${details.meta.l4.type} `
                        }
                        if('code' in details.meta.l4){
                            detailHTML += ` code: ${details.meta.l4.code} `
                        }
                        detailHTML += `<\p>`
                    }
                    if('data' in details.meta){
                        detailHTML += `<p>Data Length: ${details.meta.data}<\p>`
                    }
                    metaArea.innerHTML = detailHTML;

                    dataArea.innerHTML = `${details.data.replace(/\n/g, '<br>')}`
                })
                .catch(error => {
                    console.error('Error fetching details:', error);
                });
            });

            table.appendChild(row);
        });
        searchResults.appendChild(table);
    })
    .catch(error => {
        console.error('Error:', error);
    });
});
