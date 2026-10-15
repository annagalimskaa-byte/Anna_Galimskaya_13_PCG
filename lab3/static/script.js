const fileInput = document.getElementById('fileInput');
const methodSelect = document.getElementById('methodSelect');
const processBtn = document.getElementById('processBtn');
const saveBtn = document.getElementById('saveBtn');
const originalImg = document.getElementById('originalImg');
const processedImg = document.getElementById('processedImg');
const originalPlaceholder = document.getElementById('originalPlaceholder');
const processedPlaceholder = document.getElementById('processedPlaceholder');
const status = document.getElementById('status');

let currentFile = null;
let resultDataUrl = null;

// Загрузка изображения
fileInput.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;

    currentFile = file;
    const reader = new FileReader();

    reader.onload = (event) => {
        originalImg.src = event.target.result;
        originalImg.classList.add('visible');
        originalPlaceholder.classList.add('hidden');

        // Сброс результата
        processedImg.classList.remove('visible');
        processedPlaceholder.classList.remove('hidden');
        processedPlaceholder.textContent = 'Нажмите «Обработать»';
        saveBtn.disabled = true;
        resultDataUrl = null;

        status.textContent = `Загружено: ${file.name}`;
        status.className = 'status';
    };

    reader.readAsDataURL(file);
});

// Обработка
processBtn.addEventListener('click', async () => {
    if (!currentFile) {
        status.textContent = 'Сначала загрузите изображение';
        status.className = 'status error';
        return;
    }

    processBtn.disabled = true;
    status.textContent = 'Обработка...';
    status.className = 'status';

    const formData = new FormData();
    formData.append('image', currentFile);
    formData.append('method', methodSelect.value);

    try {
        const response = await fetch('/process', {
            method: 'POST',
            body: formData
        });

        const data = await response.json();

        if (data.error) {
            status.textContent = 'Ошибка: ' + data.error;
            status.className = 'status error';
            processBtn.disabled = false;
            return;
        }

        processedImg.src = data.result;
        processedImg.classList.add('visible');
        processedPlaceholder.classList.add('hidden');

        resultDataUrl = data.result;
        saveBtn.disabled = false;

        status.textContent = data.description;
        status.className = 'status success';

    } catch (err) {
        status.textContent = 'Ошибка соединения: ' + err.message;
        status.className = 'status error';
    } finally {
        processBtn.disabled = false;
    }
});

// Сохранение
saveBtn.addEventListener('click', () => {
    if (!resultDataUrl) return;

    const link = document.createElement('a');
    link.href = resultDataUrl;
    link.download = 'processed_' + methodSelect.value + '.png';
    link.click();
});
