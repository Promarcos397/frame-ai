/* ─────────────────────────────────────────────────────────────────────────
   Frame AI — Browser App Logic
   ────────────────────────────────────────────────────────────────────────── */

// API endpoint — configurable for Cloudflare Pages deployment.
// In local mode: points to the frame-ai --serve process.
// In deployed mode: set window.ARTDIR_API_URL in /public/_config.js
// (injected by a Cloudflare Pages Function or static config file).
const API_URL = (window.ARTDIR_API_URL || 'https://ibrahimar397-frame-ai-backend.hf.space') + '/generate';

// ── DOM refs ─────────────────────────────────────────────────────────────────
const nav           = document.getElementById('nav');
const statusDot     = document.getElementById('status-dot');
const statusLabel   = document.getElementById('status-label');
const inputText     = document.getElementById('input-text');
const charCount     = document.getElementById('char-count');
const dropZone      = document.getElementById('drop-zone');
const dropZoneInner = document.getElementById('drop-zone-inner');
const dropPreview   = document.getElementById('drop-preview');
const previewImg    = document.getElementById('preview-img');
const previewName   = document.getElementById('preview-name');
const previewRemove = document.getElementById('preview-remove');
const fileInput     = document.getElementById('file-input');
const exampleChips  = document.querySelectorAll('.example-chip');
const btnGenerate   = document.getElementById('btn-generate');
const btnText       = document.getElementById('btn-text');
const errorMessage  = document.getElementById('error-message');
const errorText     = document.getElementById('error-text');

// Output
const outputEmpty   = document.getElementById('output-empty');
const outputLoading = document.getElementById('output-loading');
const outputBrief   = document.getElementById('output-brief');
const outputActions = document.getElementById('output-actions');
const loadingSubtext= document.getElementById('loading-subtext');
const btnCopy       = document.getElementById('btn-copy');
const btnDownload   = document.getElementById('btn-download');

// Brief sections
const paletteGrid   = document.getElementById('palette-grid');
const paletteRaw    = document.getElementById('palette-raw');
const lightingText  = document.getElementById('lighting-text');
const compositionText= document.getElementById('composition-text');
const referencesList= document.getElementById('references-list');
const textureText   = document.getElementById('texture-text');
const toneText      = document.getElementById('tone-text');
const avoidList     = document.getElementById('avoid-list');

// ── State ─────────────────────────────────────────────────────────────────────
let imageBase64   = '';
let imageFilename = '';
let currentBrief  = null;
let isGenerating  = false;

// ── Nav scroll behaviour ──────────────────────────────────────────────────────
window.addEventListener('scroll', () => {
  nav.classList.toggle('scrolled', window.scrollY > 60);
}, { passive: true });

// ── Character counter ─────────────────────────────────────────────────────────
inputText.addEventListener('input', () => {
  charCount.textContent = inputText.value.length.toLocaleString();
});

// ── Server health check ───────────────────────────────────────────────────────
function setStatus(state, label) {
  statusDot.className = `status-dot ${state}`;
  statusLabel.textContent = label;
}

async function checkHealth() {
  try {
    const base = window.ARTDIR_API_URL || 'http://localhost:8080';
    const res = await fetch(`${base}/health`, { method: 'GET' });
    if (res.ok) {
      setStatus('online', 'Model ready');
    } else {
      setStatus('offline', 'Server error');
    }
  } catch {
    setStatus('offline', 'Not connected');
  }
}

setStatus('loading', 'Connecting…');
checkHealth();
setInterval(checkHealth, 30_000);

// ── Example chips ──────────────────────────────────────────────────────────────
exampleChips.forEach(chip => {
  chip.addEventListener('click', () => {
    inputText.value = chip.dataset.text;
    charCount.textContent = inputText.value.length.toLocaleString();
    inputText.focus();
    // Smooth scroll to input on mobile
    if (window.innerWidth < 1024) {
      inputText.scrollIntoView({ behavior: 'smooth', block: 'center' });
    }
  });
});

// ── Image handling ─────────────────────────────────────────────────────────────
dropZone.addEventListener('click', (e) => {
  if (e.target === previewRemove || previewRemove.contains(e.target)) return;
  fileInput.click();
});

dropZone.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' || e.key === ' ') {
    e.preventDefault();
    fileInput.click();
  }
});

fileInput.addEventListener('change', () => {
  if (fileInput.files && fileInput.files[0]) {
    handleFile(fileInput.files[0]);
  }
});

dropZone.addEventListener('dragover', (e) => {
  e.preventDefault();
  dropZone.classList.add('dragover');
});

dropZone.addEventListener('dragleave', () => {
  dropZone.classList.remove('dragover');
});

dropZone.addEventListener('drop', (e) => {
  e.preventDefault();
  dropZone.classList.remove('dragover');
  const file = e.dataTransfer.files[0];
  if (file) handleFile(file);
});

previewRemove.addEventListener('click', (e) => {
  e.stopPropagation();
  clearImage();
});

function handleFile(file) {
  const allowed = ['image/jpeg', 'image/png', 'image/bmp'];
  if (!allowed.includes(file.type)) {
    showError('Unsupported image format. Please use JPG, PNG, or BMP.');
    return;
  }
  imageFilename = file.name;

  const reader = new FileReader();
  reader.onload = (ev) => {
    imageBase64 = ev.target.result; // full data URI
    previewImg.src = imageBase64;
    previewName.textContent = file.name;
    dropZoneInner.classList.add('hidden');
    dropPreview.classList.remove('hidden');
  };
  reader.readAsDataURL(file);
}

function clearImage() {
  imageBase64   = '';
  imageFilename = '';
  previewImg.src = '';
  previewName.textContent = '';
  fileInput.value = '';
  dropPreview.classList.add('hidden');
  dropZoneInner.classList.remove('hidden');
}

// ── Generate ───────────────────────────────────────────────────────────────────
btnGenerate.addEventListener('click', generate);

inputText.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
    generate();
  }
});

const LOADING_MESSAGES = [
  'Model is thinking…',
  'Composing palette…',
  'Defining lighting…',
  'Referencing artists…',
  'Building atmosphere…',
  'Finishing brief…',
];

async function generate() {
  if (isGenerating) return;

  const text = inputText.value.trim();
  if (!text && !imageBase64) {
    showError('Please enter a concept or upload an image.');
    return;
  }

  isGenerating = true;
  hideError();
  showState('loading');

  btnGenerate.disabled = true;
  btnText.textContent = 'Generating…';

  // Cycle loading messages
  let msgIdx = 0;
  const msgInterval = setInterval(() => {
    loadingSubtext.textContent = LOADING_MESSAGES[msgIdx % LOADING_MESSAGES.length];
    msgIdx++;
  }, 3500);

  // Scroll output into view on mobile
  if (window.innerWidth < 1024) {
    document.getElementById('panel-output').scrollIntoView({ behavior: 'smooth' });
  }

  try {
    const payload = {
      text:           text,
      image_base64:   imageBase64,
      image_filename: imageFilename,
    };

    const res = await fetch(API_URL, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    });

    if (!res.ok) {
      const errBody = await res.json().catch(() => ({ error: `HTTP ${res.status}` }));
      throw new Error(errBody.error || `Server returned ${res.status}`);
    }

    const data = await res.json();
    currentBrief = data;
    renderBrief(data);
    showState('brief');

  } catch (err) {
    showState('empty');
    showError(err.message || 'Generation failed. Is the server running?');
  } finally {
    clearInterval(msgInterval);
    isGenerating = false;
    btnGenerate.disabled = false;
    btnText.textContent = 'Generate Brief';
  }
}

// ── Render brief ───────────────────────────────────────────────────────────────
function renderBrief(data) {
  // Colour palette
  renderPalette(data.colour_palette || '');

  // Text sections
  lightingText.textContent     = data.lighting_mood      || '';
  compositionText.textContent  = data.composition_style  || '';
  textureText.textContent      = data.texture_material   || '';
  toneText.textContent         = data.tone_atmosphere    || '';

  // Art references — try to parse numbered list
  renderReferences(data.art_references || '');

  // What to avoid — try to parse as bullets
  renderAvoid(data.what_to_avoid || '');

  // Show copy/save buttons
  outputActions.classList.remove('hidden');
}

function renderPalette(raw) {
  paletteGrid.innerHTML = '';
  paletteRaw.textContent = raw;

  // Extract hex codes from lines like: #1a1a2e  Name — role
  const hexRegex = /#([0-9a-fA-F]{6})\b/g;
  const matches = [...raw.matchAll(hexRegex)];

  if (matches.length === 0) return;

  matches.forEach(match => {
    const hex = '#' + match[1];
    const swatch = document.createElement('div');
    swatch.className = 'palette-swatch';
    swatch.title = `Click to copy ${hex}`;
    swatch.innerHTML = `
      <div class="swatch-block" style="background: ${hex};"></div>
      <span class="swatch-hex">${hex}</span>
    `;
    swatch.addEventListener('click', () => {
      navigator.clipboard.writeText(hex).then(() => {
        swatch.querySelector('.swatch-hex').textContent = 'Copied!';
        setTimeout(() => {
          swatch.querySelector('.swatch-hex').textContent = hex;
        }, 1200);
      });
    });
    paletteGrid.appendChild(swatch);
  });
}

function renderReferences(raw) {
  referencesList.innerHTML = '';
  const lines = raw.split('\n').filter(l => l.trim());

  lines.forEach((line, i) => {
    // Strip leading numbers/dots
    const text = line.replace(/^\d+[\.\)]\s*/, '').trim();
    if (!text) return;

    const item = document.createElement('div');
    item.className = 'reference-item';
    item.innerHTML = `
      <span class="ref-number">0${i + 1}</span>
      <span class="ref-text">${escHtml(text)}</span>
    `;
    referencesList.appendChild(item);
  });
}

function renderAvoid(raw) {
  avoidList.innerHTML = '';
  const lines = raw.split('\n').filter(l => l.trim());

  lines.forEach(line => {
    const text = line.replace(/^[-•✕✗]\s*/, '').trim();
    if (!text) return;

    const item = document.createElement('div');
    item.className = 'avoid-item';
    item.innerHTML = `
      <span class="avoid-bullet">✕</span>
      <span>${escHtml(text)}</span>
    `;
    avoidList.appendChild(item);
  });
}

function escHtml(str) {
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

// ── State management ───────────────────────────────────────────────────────────
function showState(state) {
  outputEmpty.classList.toggle('hidden',   state !== 'empty');
  outputLoading.classList.toggle('hidden', state !== 'loading');
  outputBrief.classList.toggle('hidden',   state !== 'brief');
  if (state !== 'brief') outputActions.classList.add('hidden');
}

function showError(msg) {
  errorText.textContent = msg;
  errorMessage.classList.remove('hidden');
}

function hideError() {
  errorMessage.classList.add('hidden');
}

// ── Copy & Download ────────────────────────────────────────────────────────────
btnCopy.addEventListener('click', () => {
  if (!currentBrief) return;
  const text = formatBriefPlaintext(currentBrief);
  navigator.clipboard.writeText(text).then(() => {
    btnCopy.textContent = '✓ Copied';
    setTimeout(() => { btnCopy.textContent = 'Copy'; }, 2000);
  });
});

btnDownload.addEventListener('click', () => {
  if (!currentBrief) return;
  const text = formatBriefPlaintext(currentBrief);
  const blob = new Blob([text], { type: 'text/plain' });
  const url  = URL.createObjectURL(blob);
  const a    = document.createElement('a');
  a.href     = url;
  a.download = 'frame-ai-brief.txt';
  a.click();
  URL.revokeObjectURL(url);
});

function formatBriefPlaintext(data) {
  const line = '═'.repeat(51);
  const now  = new Date().toLocaleString();
  return [
    line,
    '  FRAME AI — Visual Art Direction Brief',
    `  Generated: ${now}`,
    line,
    '',
    'COLOUR PALETTE',
    data.colour_palette || '',
    '',
    'LIGHTING MOOD',
    data.lighting_mood || '',
    '',
    'COMPOSITION STYLE',
    data.composition_style || '',
    '',
    'ART MOVEMENT REFERENCES',
    data.art_references || '',
    '',
    'TEXTURE AND MATERIAL',
    data.texture_material || '',
    '',
    'TONE AND ATMOSPHERE',
    data.tone_atmosphere || '',
    '',
    'WHAT TO AVOID',
    data.what_to_avoid || '',
    '',
    line,
  ].join('\n');
}

// ── Initial state ──────────────────────────────────────────────────────────────
showState('empty');

// ── Imagine (image generation) ────────────────────────────────────────────────
const btnImagine  = document.getElementById('btn-imagine');
const imageOutput = document.getElementById('image-output');
const imageLoading= document.getElementById('image-loading');
const generatedImg= document.getElementById('generated-img');
const btnImgDl    = document.getElementById('btn-img-dl');

btnImagine.addEventListener('click', generateImage);

function buildImagePrompt(data) {
  // Distil the brief into a rich visual prompt for Pollinations
  const parts = [];
  if (data.tone_atmosphere)   parts.push(data.tone_atmosphere.slice(0, 120));
  if (data.lighting_mood)     parts.push(data.lighting_mood.slice(0, 80));
  if (data.composition_style) parts.push(data.composition_style.slice(0, 80));
  if (data.art_references)    parts.push(data.art_references.split('\n')[0].slice(0, 80));
  // Append colour palette hex values as style hints
  const hexes = (data.colour_palette || '').match(/#[0-9a-fA-F]{6}/g) || [];
  if (hexes.length) parts.push('color palette: ' + hexes.slice(0,4).join(' '));
  parts.push('cinematic, editorial photography, highly detailed, 4k');
  return parts.join(', ');
}

async function generateImage() {
  if (!currentBrief) return;

  btnImagine.disabled = true;
  btnImagine.textContent = '⏳ Rendering…';

  imageOutput.classList.remove('hidden');
  imageLoading.classList.remove('hidden');
  generatedImg.classList.add('hidden');
  btnImgDl.classList.add('hidden');

  // Scroll to image section
  imageOutput.scrollIntoView({ behavior: 'smooth', block: 'start' });

  const prompt   = buildImagePrompt(currentBrief);
  const encoded  = encodeURIComponent(prompt);
  const seed     = Math.floor(Math.random() * 999999);
  const imageUrl = `https://image.pollinations.ai/prompt/${encoded}?width=1280&height=720&seed=${seed}&nologo=true&model=flux`;

  const img = new Image();
  img.onload = () => {
    imageLoading.classList.add('hidden');
    generatedImg.src = imageUrl;
    generatedImg.classList.remove('hidden');
    btnImgDl.href = imageUrl;
    btnImgDl.classList.remove('hidden');
    btnImagine.disabled = false;
    btnImagine.textContent = '★ Imagine';
  };
  img.onerror = () => {
    imageLoading.innerHTML = '<p style="color:var(--clr-error,#e94560)">Image generation failed — try again</p>';
    btnImagine.disabled = false;
    btnImagine.textContent = '★ Imagine';
  };
  img.src = imageUrl;
}
