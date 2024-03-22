var searchResults = document.getElementById('searchResults');

// 获取所有搜索结果行元素
var resultRows = searchResults.querySelectorAll('.searchResultRow');

// 为每个搜索结果行添加单击事件监听器
resultRows.forEach(function(row) {
    row.addEventListener('click', function() {
        // 移除所有搜索结果行的选中样式
        resultRows.forEach(function(row) {
            row.classList.remove('selected');
        });

        // 将当前单击的搜索结果行添加选中样式
        row.classList.add('selected');
    });
});