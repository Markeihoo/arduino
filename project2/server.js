require('dotenv').config();
const express = require('express');
const axios = require('axios');
const { Client, GatewayIntentBits } = require('discord.js');

const app = express();
app.use(express.json());

const WEBHOOK = process.env.DISCORD_WEBHOOK_URL;
// const BOT_TOKEN = process.env.DISCORD_BOT_TOKEN;
const OPENROUTER_KEY = process.env.OPENROUTER_API_KEY;

let lastSensor = null;
let totalReadings = 0;
let dailyReadings = [];
const startTime = new Date();

async function askAI(prompt) {
  for (const model of ['nvidia/nemotron-3-super-120b-a12b:free', 'mistralai/mistral-7b-instruct:free']) {
    try {
      const res = await axios.post(
        'https://openrouter.ai/api/v1/chat/completions&#39;,
        { model, messages: [{ role: 'user', content: prompt }] },
        { headers: { 'Authorization': `Bearer ${OPENROUTER_KEY}`, 'Content-Type': 'application/json' }, timeout: 30000 }
      );
      const text = res.data?.choices?.[0]?.message?.content?.trim();
      if (text?.length > 5) { console.log(`✅ AI: OpenClaw (${model})`); return { text, source: 'OpenClaw (Nemotron)' }; }
    } catch (e) { console.warn(`⚠️ ${model}:`, e.response?.data?.error?.message || e.message); }
  }
  return { text: 'ระบบ AI กำลังประมวลผล — กรุณาตรวจสอบค่า sensor ด้านบน', source: 'OpenClaw (Offline)' };
}

function alertOf(temp) {
  if (temp > 35) return { color: 0xff4444, label: '🔴 อันตราย', emoji: '🥵' };
  if (temp > 30) return { color: 0xffaa00, label: '🟡 ควรระวัง', emoji: '🌤️' };
  return { color: 0x44ff88, label: '🟢 ปกติ', emoji: '😊' };
}

app.post('/sensor', async (req, res) => {
  const { temperature, humidity } = req.body;
  totalReadings++;
  lastSensor = { temperature, humidity, timestamp: new Date() };
  dailyReadings.push({ temperature, humidity });
  console.log(`\n[#${totalReadings}] ${temperature}°C | ${humidity}%`);

  const ai = await askAI(`อุณหภูมิ ${temperature}°C ความชื้น ${humidity}% วิเคราะห์สั้นๆ 1-2 ประโยค ภาษาไทย พร้อมคำแนะนำ เเละบอกสภาพอากาศโดยรวมวันนี้เป็นอย่างไรบ้าง`);
  const alert = alertOf(temperature);

  try {
    const r = await axios.post(WEBHOOK, {
      username: 'ESP8266 IoT Monitor',
      embeds: [{
        title: `${alert.emoji} Sensor Report`,
        color: alert.color,
        fields: [
          { name: '🌡️ อุณหภูมิ', value: `**${temperature}°C**`, inline: true },
          { name: '💧 ความชื้น', value: `**${humidity}%**`,  nline: true },
          { name: '⚠️ ระดับ', value: alert.label, inline: true },
          { name: `🤖 ${ai.source} วิเคราะห์`, value: ai.text }
        ],
        footer: { text: `ESP8266 IoT via OpenClaw • #${totalReadings}` },
        timestamp: new Date().toISOString()
      }]
    }, { timeout: 15000 });
    console.log(`✅ Discord sent! (${r.status})`);
  } catch (e) {
    console.error('❌ Webhook:', e.message, e.response?.data);
  }

  res.sendStatus(200);
});

// ✅ แก้ตรงนี้ เพิ่ม '0.0.0.0' ให้ ESP เข้าถึงได้
app.listen(3000, '0.0.0.0', () => console.log('IoT Server running on :3000'));

async function postDailyReport() {
  if (!dailyReadings.length) return console.log('⚠️ ไม่มีข้อมูลวันนี้');
  const temps = dailyReadings.map(r => r.temperature);
  const avgT = (temps.reduce((a, b) => a + b, 0) / temps.length).toFixed(1);
  const avgH = (dailyReadings.reduce((a, b) => a + b.humidity, 0) / dailyReadings.length).toFixed(1);
  const ai = await askAI(`อุณหภูมิเฉลี่ย ${avgT}°C สูงสุด ${Math.max(...temps)}°C ต่ำสุด ${Math.min(...temps)}°C ความชื้น ${avgH}% เขียนรายงานอากาศประจำวัน 2-3 ประโยค ภาษาไทย`);
  const color = avgT > 35 ? 0xff4444 : avgT > 30 ? 0xffaa00 : 0x0099ff;

  await axios.post(WEBHOOK, {
    username: 'ESP8266 IoT Monitor',
    embeds: [{
      title: '📰 รายงานสภาพอากาศประจำวัน', color,
      description: ai.text,
      fields: [
        { name: '🌡️ เฉลี่ย', value: `${avgT}°C`, inline: true },
        { name: '🔺 สูงสุด', value: `${Math.max(...temps)}°C`, inline: true },
        { name: '🔻 ต่ำสุด', value: `${Math.min(...temps)}°C`, inline: true },
        { name: '💧 ความชื้น', value: `${avgH}%`, inline: true },
        { name: '📊 การวัด', value: `${dailyReadings.length} ครั้ง`, inline: true }
      ],
      footer: { text: 'ESP8266 IoT via OpenClaw' }, timestamp: new Date().toISOString()
    }]
  });
  console.log('✅ Daily report sent!');
  dailyReadings = [];
}

setInterval(() => {
  const now = new Date();
  if (now.getHours() === 8 && now.getMinutes() === 0) postDailyReport();
}, 60000);

if (BOT_TOKEN && BOT_TOKEN !== 'YOUR_BOT_TOKEN_HERE') {
  const bot = new Client({ intents: [GatewayIntentBits.Guilds, GatewayIntentBits.GuildMessages, GatewayIntentBits.MessageContent] });

  bot.once('clientReady', () => console.log(`online: ${bot.user.tag}`));

  bot.on('messageCreate', async (msg) => {
    if (msg.author.bot) return;
    const cmd = msg.content.trim().toLowerCase();

    if (cmd === '!help') {
      await msg.reply({
        embeds: [{
          title: 'IoT Bot', color: 0x5865F2,
          description: 'IoT Monitor',
          fields: [
            { name: '`!status`', value: 'ค่า sensor ล่าสุด' },
            { name: '`!check`', value: 'วิเคราะห์เชิงลึกด้วย OpenClaw AI' },
            { name: '`!weather`', value: 'รายงานสภาพอากาศวันนี้' ,
          { name: '`!info`', value: 'สถิติ server' }
          ],
          footer: { text: 'ESP8266 IoT via OpenClaw' }
        }]
      });
    }

    else if (cmd === '!status') {
      if (!lastSensor) return msg.reply('⚠️ ยังไม่มีข้อมูล sensor');
      const { temperature, humidity, timestamp } = lastSensor;
      const a = alertOf(temperature);
      await msg.reply({
        embeds: [{
          title: '📡 Sensor Status', color: a.color,
          fields: [
            { name: '🌡️ อุณหภูมิ', value: `**${temperature}°C**`, inline: true },
            { name: '💧 ความชื้น', value: `**${humidity}%**`, inline: true },
            { name: '⚠️ ระดับ', value: a.label, inline: true },
            { name: '🕐 เวลา', value: new Date(timestamp).toLocaleString('th-TH') }
          ],
          footer: { text: 'ESP8266 IoT Monitor' }
        }]
      });
    }

    else if (cmd === '!check') {
      if (!lastSensor) return msg.reply('⚠️ ยังไม่มีข้อมูล sensor');
      await msg.reply('กำลังวิเคราะห์...');
      const { temperature, humidity } = lastSensor;
      const ai = await askAI(`อุณหภูมิ ${temperature}°C ความชื้น ${humidity}% วิเคราะห์เชิงลึก 3-5 ประโยค ภาษาไทย พร้อมคำแนะนำ`);
      await msg.reply({
        embeds: [{
          title: ' OpenClaw วิเคราะห์', color: 0x9b59b6,
          description: ai.text,
          footer: { text: `${ai.source} | ${temperature}°C | ${humidity}%` }
        }]
      });
    }

    else if (cmd === '!weather') {
      if (!dailyReadings.length) return msg.reply('⚠️ ยังไม่มีข้อมูลวันนี้');
      await msg.reply('📰 กำลังวิเคราะห์สภาพอากาศ...');
      await postDailyReport();
      await msg.reply('✅ ส่งรายงานแล้ว ดูด้านบนได้เลย!');
    }

    else if (cmd === '!info') {
      const up = Math.floor((Date.now() - startTime) / 60000);
      await msg.reply({
        embeds: [{
          title: '📊 Server Info', color: 0x00b4d8,
          fields: [
            { name: '📈 Readings', value: `${totalReadings} ครั้ง`, inline: true },
            { name: '⏱️ Uptime', value: up < 60 ? `${up} นาที` : `${Math.floor(up / 60)} ชม. ${up % 60} นาที`, inline: true },
            { name: '🤖 AI', value: 'OpenClaw (Nemotron)', inline: true },
            { name: '📡 ESP8266', value: lastSensor ? '🟢 Online' : '🔴 Offline', inline: true }
          ],
          footer: { text: 'ESP8266 IoT via OpenClaw' }, timestamp: new Date().toISOString()
        }]
      });
    }
  });

  bot.login(BOT_TOKEN).catch(e => console.error('Bot login failed:', e.message));
} else {
  console.warn('⚠️ DISCORD_BOT_TOKEN ไม่พบ — Bot ปิดอยู่');
}
