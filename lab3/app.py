from flask import Flask, render_template, request, send_file, jsonify
from PIL import Image
import io
import base64
from algorithms import process_image

app = Flask(__name__)


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/process', methods=['POST'])
def process():
    try:
        # Получаем файл и метод
        file = request.files.get('image')
        method = request.form.get('method', 'otsu')

        if not file:
            return jsonify({'error': 'No image provided'}), 400

        # Открываем изображение
        img = Image.open(file.stream)

        # Обрабатываем
        result_array, threshold, description = process_image(img, method)

        # Преобразуем обратно в изображение
        result_img = Image.fromarray(result_array)

        # Кодируем в base64 для отдачи
        buffer = io.BytesIO()
        result_img.save(buffer, format='PNG')
        buffer.seek(0)
        result_b64 = base64.b64encode(buffer.getvalue()).decode('utf-8')

        return jsonify({
            'success': True,
            'result': f'data:image/png;base64,{result_b64}',
            'description': description,
            'threshold': threshold
        })

    except Exception as e:
        return jsonify({'error': str(e)}), 500


if __name__ == '__main__':
    app.run(debug=True, port=5000)
