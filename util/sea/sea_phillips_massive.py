import numpy as np

def phillips_spectrum(N=64, L=64, wind_speed=32.0, wind_direction=(1.0, 0.0), wave_amp=0.0002, gravity=9.81, damping=0.001):
    """
    生成 Phillips 光谱波浪数据，包括方向、角速度、振幅和初始相位。

    参数:
        N: int - 网格分辨率 (N x N)
        L: float - 网格的物理尺寸
        wind_speed: float - 风速
        wind_direction: tuple - 风向单位向量 (x, y)
        wave_amp: float - 能量缩放因子
        gravity: float - 重力加速度
        damping: float - 阻尼系数

    返回:
        freqDir: list - 波浪方向 (vec2)
        freqOmega: list - 波浪角速度
        freqAmp: list - 波浪振幅
        freqPhaseOffset: list - 初始相位偏移
    """
    wind_dir = np.array(wind_direction) / np.linalg.norm(wind_direction)  # 风向归一化
    L_wind = (wind_speed ** 2) / gravity  # 特征波长 L_wind
    l2 = L_wind ** 2 * damping ** 2       # 阻尼长度平方

    freqDir = []
    freqOmega = []
    freqAmp = []
    freqPhaseOffset = []

    for m_prime in range(N):
        for n_prime in range(N):
            # 计算波矢量 k
            kx = np.pi * (2 * n_prime - N) / L
            kz = np.pi * (2 * m_prime - N) / L
            k = np.array([kx, kz])
            k_length = np.linalg.norm(k)

            # 过滤掉 k = 0
            if k_length < 1e-6:
                continue

            k_unit = k / k_length  # 单位波矢量
            k_dot_wind = np.dot(k_unit, wind_dir)  # 波矢量与风向的点积

            # 波浪与风向反向时能量为 0
            if k_dot_wind < 0:
                continue

            # Phillips 光谱公式
            k_length2 = k_length ** 2
            k_length4 = k_length2 ** 2
            phillips_value = wave_amp * (np.exp(-1 / (k_length2 * L_wind ** 2)) / k_length4) * (k_dot_wind ** 6) * np.exp(-k_length2 * l2)

            # 过滤能量过小的波浪
            if phillips_value < 1e-8:
                continue

            # 保存数据
            freqDir.append((round(k_unit[0], 6), round(k_unit[1], 6)))   # 波浪方向 (vec2)
            freqOmega.append(round(np.sqrt(gravity * k_length), 6))     # 角速度 ω = sqrt(g * |k|)
            freqAmp.append(round(np.sqrt(phillips_value), 6))           # 振幅取 Phillips 值的平方根
            freqPhaseOffset.append(round(np.random.uniform(0.0, 2 * np.pi), 6))  # 初始相位偏移 [0, 2π]

    return freqDir, freqOmega, freqAmp, freqPhaseOffset

def format_glsl_array(name, data, is_vector=False):
    """将数据格式化为 GLSL 数组"""
    output = f"const {'vec2' if is_vector else 'float'} {name}[{len(data)}] = {'vec2[](' if is_vector else 'float[]('}\n"
    lines = []
    count = 4  # 每行 4 个元素
    if is_vector:
        for i in range(0, len(data), count):
            line = ", ".join([f"vec2({x[0]}, {x[1]})" for x in data[i:i+count]])
            lines.append(f"    {line}")
    else:
        for i in range(0, len(data), count):
            line = ", ".join([str(x) for x in data[i:i+count]])
            lines.append(f"    {line}")
    output += ",\n".join(lines) + "\n);\n"
    return output

# 参数
N = 64  # 网格分辨率
L = 64  # 网格的物理尺寸
freqDir, freqOmega, freqAmp, freqPhaseOffset = phillips_spectrum(N=N, L=L)

# 输出 GLSL 数组
glsl_freq_count = f"const int freqCount = {len(freqDir)};\n"
glsl_freq_dir = format_glsl_array("freqDir", freqDir, is_vector=True)
glsl_freq_omega = format_glsl_array("freqOmega", freqOmega)
glsl_freq_amp = format_glsl_array("freqAmp", freqAmp)
glsl_freq_phase = format_glsl_array("freqPhaseOffset", freqPhaseOffset)

# 打印结果
print(glsl_freq_count)
print(glsl_freq_dir)
print(glsl_freq_omega)
print(glsl_freq_amp)
print(glsl_freq_phase)
