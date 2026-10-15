import numpy as np
from PIL import Image


def to_grayscale(img):
    """Преобразование в grayscale"""
    return np.array(img.convert('L'))


def otsu_threshold(gray):
    """Метод Оцу — автоматический порог по гистограмме"""
    hist, _ = np.histogram(gray, bins=256, range=(0, 256))
    total = gray.size
    sum_total = np.dot(np.arange(256), hist)

    sum_b = 0.0
    w_b = 0
    max_variance = 0.0
    best_threshold = 0

    for t in range(256):
        w_b += hist[t]
        if w_b == 0:
            continue
        w_f = total - w_b
        if w_f == 0:
            break
        sum_b += t * hist[t]
        m_b = sum_b / w_b
        m_f = (sum_total - sum_b) / w_f
        variance = w_b * w_f * (m_b - m_f) ** 2
        if variance > max_variance:
            max_variance = variance
            best_threshold = t

    return best_threshold


def iterative_threshold(gray):
    """Итеративный метод — пошаговое уточнение порога"""
    hist, _ = np.histogram(gray, bins=256, range=(0, 256))
    total = gray.size
    sum_total = np.dot(np.arange(256), hist)

    T = sum_total / total

    for _ in range(100):
        sum1 = 0.0
        count1 = 0
        sum2 = 0.0
        count2 = 0

        for i in range(256):
            if i < T:
                sum1 += i * hist[i]
                count1 += hist[i]
            else:
                sum2 += i * hist[i]
                count2 += hist[i]

        m1 = sum1 / count1 if count1 > 0 else 0
        m2 = sum2 / count2 if count2 > 0 else 0
        new_T = (m1 + m2) / 2.0

        if abs(new_T - T) < 0.5:
            T = new_T
            break
        T = new_T

    return int(T)


def adaptive_threshold(gray, window=15, C=5):
    """Адаптивный метод — локальный порог для каждого пикселя"""
    h, w = gray.shape
    result = np.zeros_like(gray)
    half = window // 2

    # Используем свёртку для быстрого вычисления среднего
    from scipy.ndimage import uniform_filter
    try:
        local_mean = uniform_filter(gray.astype(float), size=window)
    except ImportError:
        # Fallback: без scipy
        local_mean = np.zeros_like(gray, dtype=float)
        for y in range(h):
            for x in range(w):
                y1 = max(0, y - half)
                y2 = min(h, y + half + 1)
                x1 = max(0, x - half)
                x2 = min(w, x + half + 1)
                local_mean[y, x] = gray[y1:y2, x1:x2].mean()

    local_threshold = local_mean - C
    result = np.where(gray > local_threshold, 255, 0).astype(np.uint8)

    return result


def process_image(img, method):
    """Главная функция обработки"""
    gray = to_grayscale(img)

    if method == 'otsu':
        threshold = otsu_threshold(gray)
        result = np.where(gray > threshold, 255, 0).astype(np.uint8)
        return result, threshold, f'Оцу (глобальный), порог = {threshold}'
    elif method == 'iterative':
        threshold = iterative_threshold(gray)
        result = np.where(gray > threshold, 255, 0).astype(np.uint8)
        return result, threshold, f'Итеративный (глобальный), порог = {threshold}'
    elif method == 'adaptive':
        result = adaptive_threshold(gray, window=15, C=5)
        return result, None, 'Адаптивный (окно 15, C=5)'
    else:
        raise ValueError(f'Unknown method: {method}')
