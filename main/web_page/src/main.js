import './style.css';
import { Chart } from 'chart.js/auto';

console.log("Dashboard do Analisador de Energia ativo!");

// 1. CAPTURA DOS ELEMENTOS DE TEXTO (Grandezas)
const elTensao = document.getElementById('val-tensao');
const elCorrente = document.getElementById('val-corrente');
const elPotencia = document.getElementById('val-potencia');
const elStatus = document.getElementById('status-chip');

function requisitarGrandezasMedidor() {
  fetch('/api/v1/dados')
    .then(response => {
      if (!response.ok) throw new Error(`Erro na rede: ${response.status}`);
      return response.json();
    })
    .then(dados => {
      if (dados.tensao !== undefined && elTensao) elTensao.innerHTML = `${dados.tensao.toFixed(1)} <span class="unit">V</span>`;
      if (dados.corrente !== undefined && elCorrente) elCorrente.innerHTML = `${dados.corrente.toFixed(2)} <span class="unit">A</span>`;
      if (dados.potencia !== undefined && elPotencia) elPotencia.innerHTML = `${dados.potencia.toFixed(1)} <span class="unit">W</span>`;
      
      if (elStatus) {
        elStatus.textContent = "Conectado";
        elStatus.style.backgroundColor = "var(--cor-sucesso)";
      }
    })
    .catch(erro => {
      console.error("Falha ao atualizar dados da API:", erro);
      if (elStatus) {
        elStatus.textContent = "Desconectado";
        elStatus.style.backgroundColor = "#ef4444";
      }
    });
}


requisitarGrandezasMedidor();
setInterval(requisitarGrandezasMedidor, 5000);

const ctx = document.getElementById('graficoPotencia').getContext('2d');
const graficoHarmonicas = new Chart(ctx, {
  type: 'bar', 
  data: {
    
    labels: ['Fundamental (60Hz)', '2ª Ordem', '3ª Ordem', '4ª Ordem', '5ª Ordem'],
    datasets: [
      {
        label: 'Tensão (%)',
        data:[],
        backgroundColor: 'rgba(56, 189, 248, 0.6)', 
        borderColor: '#38bdf8',
        borderWidth: 1
      },
      {
        label: 'Corrente (%)',
        data:[],
        backgroundColor: 'rgba(16, 185, 129, 0.6)', 
        borderColor: '#10b981',
        borderWidth: 1
      }
    ]
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        labels: { color: '#f8fafc' } 
      }
    },
    scales: {
      x: {
        grid: { color: '#334155' }, 
        ticks: { color: '#94a3b8' }
      },
      y: {
        min: 0,
        max: 110, 
        grid: { color: '#334155' },
        ticks: { color: '#94a3b8' }
      }
    }
  }
});

function requisitarHarmonicasFFT() {
  fetch('/api/v1/fft')
    .then(response => {
      if (!response.ok) throw new Error("Erro ao buscar FFT");
      return response.json();
    })
    .then(dados => {

      if (dados.tensao_harm && dados.corrente_harm) {
        graficoHarmonicas.data.datasets[0].data = dados.tensao_harm;
        graficoHarmonicas.data.datasets[1].data = dados.corrente_harm;
        

        graficoHarmonicas.update();
      }
    })
    .catch(erro => console.error("Falha ao atualizar gráfico de harmônicas:", erro));
}


requisitarHarmonicasFFT();


setInterval(requisitarHarmonicasFFT, 5000);
