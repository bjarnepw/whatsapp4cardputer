// server.js
const express = require('express');
const { Client, LocalAuth } = require('whatsapp-web.js');
const qrcode = require('qrcode-terminal');
const bodyParser = require('body-parser');

const app = express();
app.use(bodyParser.json());

// In-memory storage for chats/messages
// messages[phone] = [{ from: 'me'|'them', text, time }]
const messages = {};

const contactNames = {};

function addMessage(phone, entry) {
    if (!messages[phone]) messages[phone] = [];
    messages[phone].push(entry);
}

// WhatsApp client
const client = new Client({
    authStrategy: new LocalAuth({ clientId: "cardputer" }),
    puppeteer: {
        headless: true,
        args: ['--no-sandbox', '--disable-setuid-sandbox']
    }
});

client.on('qr', (qr) => {
    console.log('Scan this QR code in WhatsApp (Linked devices):');
    qrcode.generate(qr, { small: true });
});


client.on('ready', async() => {
    console.log('✅ WhatsApp Client Ready!');

    // 1. Kontakte laden
    console.log('Loading Contacts...');
    try {
        const contacts = await client.getContacts();
        let loadedContacts = 0;
        for (const contact of contacts) {

            if (contact.isMyContact && contact.name && contact.number) {

                contactNames[contact.number] = contact.name;
                loadedContacts++;
            }
        }
        console.log(`✅ ${loadedContacts} Contacts loaded and names assigned.`);
    } catch (e) {
        console.error('Error loading contacts:', e);
    }

    console.log('Loading Chats (this can take a while)...');


    const messageLimit = null; //loads infinit messages hopefully 

    try {
        const chats = await client.getChats();
        for (const chat of chats) {
            if (chat.isGroup) {
                continue;
            }

            const phone = chat.id.user;

            const chatMessages = await chat.fetchMessages({ limit: messageLimit });

            for (const msg of chatMessages.reverse()) {
                const isMe = msg.fromMe || false;

                const timestampMs = msg.timestamp * 1000;

                addMessage(phone, {
                    from: isMe ? 'me' : 'them',
                    text: msg.body,
                    time: timestampMs
                });
            }
        }
        console.log(`✅ Chat (last ${messageLimit} messages) for ${chats.length} loaded.`);
    } catch (e) {
        console.error('Fehler beim Laden der Chatverläufe:', e);
    }
});


client.on('auth_failure', (msg) => {
    console.error('Auth failure', msg);
});

client.on('message', msg => {
    // msg.from looks like '4917xxxxxxx@c.us' or 'XXXXXXXXXXX@g.us' for groups
    const raw = msg.from.split('@')[0];

    // Ignoriere Status-Updates etc.
    if (!msg.from.endsWith('@c.us')) {
        return;
    }

    const phone = raw;
    const isMe = msg.fromMe || false;

    // store incoming message
    addMessage(phone, { from: isMe ? 'me' : 'them', text: msg.body, time: Date.now() });

    // Versuche, den Namen zu loggen
    const contactName = contactNames[phone] || phone;
    console.log(`📩 ${isMe ? 'me' : contactName}: ${msg.body}`);
});

// Express endpoints

// GET /chats -> list of chats with last message preview
app.get('/chats', (req, res) => {
    const list = Object.keys(messages).map(phone => {
        const arr = messages[phone];
        const last = arr && arr.length ? arr[arr.length - 1] : null;
        return {
            phone,
            name: contactNames[phone] || phone, // Dies funktioniert jetzt automatisch
            lastText: last ? last.text : '',
            lastTime: last ? last.time : 0,
            count: arr ? arr.length : 0
        };
    }).sort((a, b) => b.lastTime - a.lastTime); // newest first
    res.json(list);
});

// GET /chat/:phone -> full messages for that phone
app.get('/chat/:phone', (req, res) => {
    const phone = req.params.phone;
    res.json(messages[phone] || []);
});

// POST /send -> { to: "+4917...", text: "hello" }
app.post('/send', async(req, res) => {
    try {
        const { to, text } = req.body;
        if (!to || !text) return res.status(400).json({ error: 'Missing to/text' });

        const chatId = to.replace('+', '') + '@c.us';
        await client.sendMessage(chatId, text);

        // store outgoing message
        addMessage(to.replace('+', ''), { from: 'me', text, time: Date.now() });
        console.log(`Sent to ${to}: ${text}`);
        res.json({ ok: true });
    } catch (e) {
        console.error('Send error', e);
        res.status(500).json({ error: e.toString() });
    }
});

// Simple health check
app.get('/', (req, res) => res.send('WhatsApp Cardputer Server'));

// Start server + WhatsApp client
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`📡 Server running on port ${PORT}`);
    client.initialize();
});