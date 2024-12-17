import numpy as np

def generate_wave_data(num_frequencies=64):
    """生成波浪数据：方向、角速度、振幅和初始相位"""
    freq_dir = []
    freq_omega = []
    freq_amp = []
    freq_phase = []

    # 生成数据
    for _ in range(num_frequencies):
        # 方向：随机二维单位向量
        theta = np.random.uniform(0, 2 * np.pi)
        direction = (round(np.cos(theta), 3), round(np.sin(theta), 3))
        freq_dir.append(direction)
        
        # 角速度：随机范围 [0.5, 1.5]
        omega = round(np.random.uniform(0.5, 1.5), 3)
        freq_omega.append(omega)
        
        # 振幅：随机范围 [0.01, 0.06]
        amp = round(np.random.uniform(0.01, 0.06), 3)
        freq_amp.append(amp)
        
        # 初始相位：随机范围 [0, 2.5]
        phase = round(np.random.uniform(0.0, 2.5), 3)
        freq_phase.append(phase)

    return freq_dir, freq_omega, freq_amp, freq_phase

def format_glsl_array(name, data, is_vector=False):
    """将数据格式化为 GLSL 数组，方便复制到 Shader 里"""
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

# 生成数据
num_frequencies = 64
freq_dir, freq_omega, freq_amp, freq_phase = generate_wave_data(num_frequencies)

# 格式化为 GLSL 数组
glsl_freq_count = f"const int freqCount = {num_frequencies};\n"
glsl_freq_dir = format_glsl_array("freqDir", freq_dir, is_vector=True)
glsl_freq_omega = format_glsl_array("freqOmega", freq_omega)
glsl_freq_amp = format_glsl_array("freqAmp", freq_amp)
glsl_freq_phase = format_glsl_array("freqPhaseOffset", freq_phase)

# 输出结果
print(glsl_freq_count)
print(glsl_freq_dir)
print(glsl_freq_omega)
print(glsl_freq_amp)
print(glsl_freq_phase)
