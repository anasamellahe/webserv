// Upload manager: list files, upload via AJAX, delete files with DELETE
function setStatus(msg, isError){
  const el = document.getElementById('uploadStatus');
  if (!el) return;
  el.textContent = msg;
  el.style.color = isError ? '#b91c1c' : '';
}

function fetchUploadsList(){
  const list = document.getElementById('uploadsList');
  if (!list) return;
  list.innerHTML = '<li class="muted">Loading...</li>';
  fetch('/uploads')
    .then(res => {
      if (!res.ok) throw new Error('Directory listing not available');
      return res.text();
    })
    .then(html => {
      // crude parsing: look for links in returned HTML
      const tmp = document.createElement('div');
      tmp.innerHTML = html;
      const anchors = tmp.querySelectorAll('a');
      list.innerHTML = '';
      if (anchors.length === 0) {
        list.innerHTML = '<li class="muted">No files found</li>';
        return;
      }
      anchors.forEach(a => {
        const href = a.getAttribute('href') || a.textContent;
        const name = href.split('/').pop();
        const li = document.createElement('li');
        const left = document.createElement('div');
        left.innerHTML = '<a href="' + href + '">' + name + '</a>';
        const actions = document.createElement('div');
        actions.className = 'file-actions';
        const dl = document.createElement('a');
        dl.href = href; dl.textContent = 'Download'; dl.className = 'muted'; dl.style.marginRight='8px';
        const delBtn = document.createElement('button');
        delBtn.textContent = 'Delete';
        delBtn.addEventListener('click', function(){
          if (!confirm('Delete "' + name + '"?')) return;
          setStatus('Deleting ' + name + '...');
          fetch('/uploads/' + encodeURIComponent(name), { method: 'DELETE' })
            .then(r => r.text())
            .then(t => { setStatus('Delete response: ' + t); fetchUploadsList(); })
            .catch(e => setStatus('Delete failed: ' + e, true));
        });
        actions.appendChild(dl);
        actions.appendChild(delBtn);
        li.appendChild(left);
        li.appendChild(actions);
        list.appendChild(li);
      });
    })
    .catch(err => {
      list.innerHTML = '<li class="muted">Directory listing not available</li>';
      setStatus('Could not fetch uploads: ' + err, true);
    });
}

function uploadFile(file){
  const form = new FormData();
  form.append('file', file);
  setStatus('Uploading ' + file.name + '...');
  fetch('/uploads', { method: 'POST', body: form })
    .then(r => r.text())
    .then(t => { setStatus('Upload response: ' + t); fetchUploadsList(); })
    .catch(e => setStatus('Upload failed: ' + e, true));
}

document.addEventListener('DOMContentLoaded', function(){
  // Bind upload form
  const form = document.getElementById('uploadForm');
  const input = document.getElementById('uploadFile');
  if (form && input){
    form.addEventListener('submit', function(ev){
      ev.preventDefault();
      if (input.files.length === 0){ setStatus('Select a file first', true); return; }
      uploadFile(input.files[0]);
    });
  }

  // Initial list fetch
  fetchUploadsList();
});
