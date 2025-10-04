import numpy as np
import matplotlib.pyplot as plt

# Carrega os dados do arquivo CSV
# O arquivo tem formato: Amostra,Accel_X,Accel_Y,Accel_Z,Gyro_X,Gyro_Y,Gyro_Z

# Pula a primeira linha (cabeçalho) e carrega os dados
data = np.loadtxt('accel_gyro_samples.csv', delimiter=',', skiprows=1)
print("Dados carregados com sucesso!")
print(f"Shape dos dados: {data.shape}")
print(f"Primeiras 5 linhas:")
print(data[:5])

# Extrai os dados (colunas: Amostra, Accel_X, Accel_Y, Accel_Z, Gyro_X, Gyro_Y, Gyro_Z)
amostras = data[:, 0]
accel_x = data[:, 1]
accel_y = data[:, 2] 
accel_z = data[:, 3]
gyro_x = data[:, 4]
gyro_y = data[:, 5]
gyro_z = data[:, 6]

# Cria um tempo baseado nas amostras (assumindo 100ms entre amostras)
tempo = amostras * 0.1  # 100ms = 0.1s

# Cria subplots para organizar os gráficos
fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))

# Gráfico 1: Acelerômetro (3 eixos)
ax1.plot(tempo, accel_x, 'r-', label='Accel X', linewidth=1.5)
ax1.plot(tempo, accel_y, 'g-', label='Accel Y', linewidth=1.5)
ax1.plot(tempo, accel_z, 'b-', label='Accel Z', linewidth=1.5)
ax1.set_title('Dados do Acelerômetro (MPU6050)', fontsize=14, fontweight='bold')
ax1.set_xlabel('Tempo (s)')
ax1.set_ylabel('Aceleração (LSB)')
ax1.legend()
ax1.grid(True, alpha=0.3)

# Gráfico 2: Giroscópio (3 eixos)
ax2.plot(tempo, gyro_x, 'r-', label='Gyro X', linewidth=1.5)
ax2.plot(tempo, gyro_y, 'g-', label='Gyro Y', linewidth=1.5)
ax2.plot(tempo, gyro_z, 'b-', label='Gyro Z', linewidth=1.5)
ax2.set_title('Dados do Giroscópio (MPU6050)', fontsize=14, fontweight='bold')
ax2.set_xlabel('Tempo (s)')
ax2.set_ylabel('Velocidade Angular (LSB)')
ax2.legend()
ax2.grid(True, alpha=0.3)

# Gráfico 3: Magnitude da Aceleração
accel_magnitude = np.sqrt(accel_x**2 + accel_y**2 + accel_z**2)
ax3.plot(tempo, accel_magnitude, 'purple', linewidth=2)
ax3.set_title('Magnitude da Aceleração', fontsize=14, fontweight='bold')
ax3.set_xlabel('Tempo (s)')
ax3.set_ylabel('Magnitude (LSB)')
ax3.grid(True, alpha=0.3)

# Gráfico 4: Magnitude da Velocidade Angular
gyro_magnitude = np.sqrt(gyro_x**2 + gyro_y**2 + gyro_z**2)
ax4.plot(tempo, gyro_magnitude, 'orange', linewidth=2)
ax4.set_title('Magnitude da Velocidade Angular', fontsize=14, fontweight='bold')
ax4.set_xlabel('Tempo (s)')
ax4.set_ylabel('Magnitude (LSB)')
ax4.grid(True, alpha=0.3)

# Ajusta o layout para evitar sobreposição
plt.tight_layout()

# Adiciona um título geral
fig.suptitle('Análise dos Dados do MPU6050', fontsize=16, fontweight='bold', y=0.98)

# Ajusta o espaçamento superior para não sobrepor o título
plt.subplots_adjust(top=0.9)

# Exibe o gráfico
plt.show()

# Estatísticas básicas
print("\n=== ESTATÍSTICAS DOS DADOS ===")
print(f"Total de amostras: {len(data)}")
print(f"Duração da coleta: {tempo[-1]:.2f} segundos")
print(f"Taxa de amostragem: {1/0.1:.1f} Hz")

print("\nACELERÔMETRO:")
print(f"  X - Min: {accel_x.min():8.0f}, Max: {accel_x.max():8.0f}, Média: {accel_x.mean():8.1f}")
print(f"  Y - Min: {accel_y.min():8.0f}, Max: {accel_y.max():8.0f}, Média: {accel_y.mean():8.1f}")
print(f"  Z - Min: {accel_z.min():8.0f}, Max: {accel_z.max():8.0f}, Média: {accel_z.mean():8.1f}")

print("\nGIROSCÓPIO:")
print(f"  X - Min: {gyro_x.min():8.0f}, Max: {gyro_x.max():8.0f}, Média: {gyro_x.mean():8.1f}")
print(f"  Y - Min: {gyro_y.min():8.0f}, Max: {gyro_y.max():8.0f}, Média: {gyro_y.mean():8.1f}")
print(f"  Z - Min: {gyro_z.min():8.0f}, Max: {gyro_z.max():8.0f}, Média: {gyro_z.mean():8.1f}")