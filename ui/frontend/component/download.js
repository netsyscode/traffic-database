var downloadButton = document.getElementById('downloadButton');

downloadButton.addEventListener('click', function() {
    // 向后端发送请求下载文件
    fetch(window.BackendURL+'/download', {
        method: 'GET',
    })
    .then(response => response.blob()) // 将响应数据转换为 Blob 对象
    .then(blob => {
        // 创建一个 URL 对象，用于生成下载链接
        var url = window.URL.createObjectURL(blob);
        // 创建一个 <a> 元素并设置其下载链接
        var a = document.createElement('a');
        a.href = url;
        // 设置下载的文件名
        a.download = 'result.pcap'; // 替换为实际的文件名
        // 触发点击事件以开始下载
        a.click();
        // 释放 URL 对象
        window.URL.revokeObjectURL(url);
    })
    .catch(error => {
        console.error('Error:', error);
    });
});