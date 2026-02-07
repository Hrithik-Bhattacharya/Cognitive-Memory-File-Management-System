import React, { useState, useEffect, useMemo, useRef } from 'react';

function App() {
  const [files, setFiles] = useState([]);
  const [logs, setLogs] = useState([]);
  const [socket, setSocket] = useState(null);

  const [searchQuery, setSearchQuery] = useState("");
  const [searchResults, setSearchResults] = useState([]);
  const [customKeys, setCustomKeys] = useState(new Set(["Important", "Draft", "Source"]));
  const [currentFile, setCurrentFile] = useState({ name: '', content: '' });
  const [newName, setNewName] = useState("");
  const [newContent, setNewContent] = useState("");

  const fileInputRef = useRef(null);
  const folderInputRef = useRef(null);

  useEffect(() => {
    const ws = new WebSocket("ws://localhost:3001");
    ws.onopen = () => ws.send(JSON.stringify({ action: "LIST" }));
    ws.onmessage = (e) => {
      try {
        const res = JSON.parse(e.data);

        if (res.files || res.action === "LIST") {
          const normalizedFiles = res.files.map(f =>
            typeof f === 'string' ? { name: f, tags: [] } : { ...f, tags: f.tags || [] }
          );
          if (res.message?.toLowerCase().includes("search")) {
            setSearchResults(normalizedFiles);
          } else {
            setFiles(normalizedFiles);
            setSearchResults([]);
          }
        }

        if (res.content !== undefined) {
          setCurrentFile({ name: res.file || "Loaded File", content: res.content });
        }

        const time = new Date().toLocaleTimeString().split(' ')[0];
        setLogs(prev => [{ msg: `[${time}] ${res.message}`, status: res.status }, ...prev.slice(0, 49)]);
      } catch (error) { console.error("Parse Error:", error); }
    };
    setSocket(ws);
    return () => ws.close();
  }, []);

  const refresh = () => {
    setSearchResults([]);
    setSearchQuery("");
    socket?.send(JSON.stringify({ action: "LIST" }));
  }

  const downloadFile = () => {
    if (!currentFile.content) return;
    const blob = new Blob([currentFile.content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = currentFile.name.split('/').pop();
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  };

  const handleFileUpload = (e, isFolder = false) => {
    const uploadedFiles = Array.from(e.target.files);
    uploadedFiles.forEach(file => {
      const reader = new FileReader();
      reader.onload = (ev) => {
        socket.send(JSON.stringify({
          action: "WRITE",
          file: isFolder ? file.webkitRelativePath : file.name,
          data: ev.target.result
        }));
      };
      reader.readAsText(file);
    });
    setTimeout(refresh, 600);
  };

  const sortedFiles = useMemo(() => {
    const list = searchResults.length > 0 ? searchResults : files;
    return [...list].sort((a, b) => a.name.localeCompare(b.name));
  }, [files, searchResults]);

  const getFolderStructure = () => {
    const structure = { _files: [], _path: "" };
    sortedFiles.forEach(fileObj => {
      const parts = fileObj.name.split('/');
      let current = structure;
      parts.forEach((part, i) => {
        if (i === parts.length - 1) current._files.push(fileObj);
        else {
          const pathSoFar = parts.slice(0, i + 1).join('/');
          if (!current[part]) current[part] = { _files: [], _path: pathSoFar };
          current = current[part];
        }
      });
    });
    return structure;
  };

  const renderFolder = (name, data, depth = 0) => {
    const isRoot = name === 'root';
    return (
      <div key={name + depth} style={{ marginLeft: depth * 15 }}>
        {!isRoot && <div style={s.folderHeader}>📂 {name.toUpperCase()}</div>}
        {Object.keys(data).filter(k => !k.startsWith('_')).map(dir => renderFolder(dir, data[dir], depth + 1))}
        {data._files.map(f => {
          const tagCount = f.tags?.length || 0;
          const healthColor = tagCount < 3 ? '#10b981' : tagCount < 5 ? '#f59e0b' : '#ef4444';

          return (
            <div key={f.name} style={s.fileRow}>
              <div style={{ flex: 1, display: 'flex', alignItems: 'center', gap: '10px' }}>
                <span style={{ color: '#18181b', fontWeight: 'bold' }}>{f.name.split('/').pop()}</span>
                <div style={s.healthBar}>
                  {[...Array(5)].map((_, i) => (
                    <div key={i} style={{ ...s.slot, backgroundColor: i < tagCount ? healthColor : '#e2e8f0' }} />
                  ))}
                </div>
              </div>
              <div style={s.fileActions}>
                <button onClick={() => socket.send(JSON.stringify({ action: "READ", file: f.name }))} style={s.actionBtn}>Read</button>
                <button onClick={() => {
                  if (tagCount >= 5) { alert("ERROR: Slot Limit Reached (Max 5)"); return; }
                  const tag = prompt("Enter Tag:");
                  if (tag) {
                    socket.send(JSON.stringify({ action: "TAG", file: f.name, key: tag }));
                    setCustomKeys(prev => new Set(prev).add(tag));

                    setTimeout(() => {
                      socket.send(JSON.stringify({ action: "LIST" }));
                    }, 300);
                  }
                }} style={{ ...s.actionBtn, color: '#a855f7' }}>Tag</button>
                <button onClick={() => {
                  socket.send(JSON.stringify({ action: "DELETE", file: f.name }));
                  setTimeout(() => socket.send(JSON.stringify({ action: "LIST" })), 200);
                }} style={{ ...s.actionBtn, color: '#ef4444' }}>Del</button>
              </div>
            </div>
          );
        })}
      </div>
    );
  };

  return (
    <div style={s.container}>
      <header style={s.header}>
        <h1 style={s.mainTitle}>CMFS <span style={{ fontWeight: 300, color: '#60a5fa' }}>Temporal Explorer</span></h1>
        <div style={s.statsBox}>FILES_STORED: {files.length}</div>
      </header>

      <div style={s.mainGrid}>
        <div style={s.leftCol}>
          <div style={s.glassCard}>
            <h3 style={s.cardTitle}>Data Ingress</h3>
            <div style={{ display: 'flex', gap: '10px', marginBottom: '10px' }}>
              <button onClick={() => fileInputRef.current.click()} style={s.saveBtn}>Upload File</button>
              <button onClick={() => folderInputRef.current.click()} style={s.saveBtn}>Upload Folder</button>
            </div>
            <input type="file" ref={fileInputRef} onChange={(e) => handleFileUpload(e, false)} style={{ display: 'none' }} />
            <input type="file" ref={folderInputRef} onChange={(e) => handleFileUpload(e, true)} webkitdirectory="true" style={{ display: 'none' }} />
            <input placeholder="File Path" value={newName} onChange={e => setNewName(e.target.value)} style={{ ...s.input, width: '100%', marginBottom: '5px' }} />
            <textarea placeholder="Manual content entry..." value={newContent} onChange={e => setNewContent(e.target.value)} style={s.textarea} />
            <button onClick={() => { socket.send(JSON.stringify({ action: "WRITE", file: newName, data: newContent })); setNewName(""); setNewContent(""); setTimeout(refresh, 300); }} style={{ ...s.saveBtn, width: '100%', marginTop: '5px', background: '#2563eb' }}>Commit</button>
          </div>

          <div style={s.glassCard}>
            <div style={{ maxHeight: '500px', overflowY: 'auto' }}>
              {renderFolder('root', getFolderStructure())}
            </div>
          </div>
        </div>

        <div style={s.rightCol}>
          <div style={s.glassCard}>
            <h3 style={s.cardTitle}>Keyword Search</h3>
            <div style={{ display: 'flex', gap: '8px', marginBottom: '12px' }}>
              <input
                placeholder="Find tag..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && socket.send(JSON.stringify({ action: "SEARCH_KEY", key: searchQuery }))}
                style={{ ...s.input, flex: 1 }}
              />
              <button onClick={() => socket.send(JSON.stringify({ action: "SEARCH_KEY", key: searchQuery }))} style={{ ...s.saveBtn, width: 'auto' }}>Search</button>
              <button onClick={refresh} style={{ ...s.saveBtn, width: 'auto', background: '#3f3f46' }}>Clear</button>
            </div>
            <div style={{ display: 'flex', flexWrap: 'wrap', gap: '6px' }}>
              {Array.from(customKeys).map(tag => (
                <button key={tag} onClick={() => { setSearchQuery(tag); socket.send(JSON.stringify({ action: "SEARCH_KEY", key: tag })); }} style={s.tagBubble}>#{tag}</button>
              ))}
            </div>
          </div>

          {currentFile.name && (
            <div style={s.previewCard}>
              <div style={s.previewHeader}>
                <span>CONTENT: {currentFile.name}</span>
                <button onClick={downloadFile} style={s.downloadBtn}>Download File</button>
              </div>
              <div style={s.previewBody}>{currentFile.content}</div>
            </div>
          )}

          <div style={s.sidePanel}>
            <h4 style={s.sideTitle}>LOG_STREAM</h4>
            <div style={s.logContainer}>
              {logs.map((log, i) => <div key={i} style={{ ...s.logItem, color: log.status === 'error' ? '#fb7185' : '#10b981' }}>{log.msg}</div>)}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

const s = {
  container: { backgroundColor: '#ffffff', color: '#18181b', minHeight: '100vh', padding: '30px', fontFamily: 'monospace' },
  header: { borderBottom: '2px solid #e4e4e7', paddingBottom: '15px', marginBottom: '20px', display: 'flex', justifyContent: 'space-between' },
  mainTitle: { margin: 0, fontSize: '20px', color: '#111827' },
  statsBox: { background: '#f4f4f5', padding: '5px 15px', borderRadius: '4px', border: '1px solid #e4e4e7', color: '#2563eb', fontSize: '12px' },
  mainGrid: { display: 'grid', gridTemplateColumns: '1fr 380px', gap: '20px' },
  leftCol: { display: 'flex', flexDirection: 'column', gap: '20px' },
  rightCol: { display: 'flex', flexDirection: 'column', gap: '20px' },
  glassCard: { background: '#ffffff', border: '1px solid #e4e4e7', borderRadius: '8px', padding: '15px', boxShadow: '0 1px 3px 0 rgba(0, 0, 0, 0.1)' },
  cardTitle: { marginTop: 0, fontSize: '10px', color: '#71717a', marginBottom: '10px' },
  input: { background: '#ffffff', border: '1px solid #d1d5db', color: '#1f2937', padding: '8px', borderRadius: '4px' },
  textarea: { width: '100%', height: '60px', background: '#ffffff', border: '1px solid #d1d5db', color: '#1f2937', padding: '10px', borderRadius: '4px' },
  saveBtn: { background: '#3f3f46', color: 'white', border: 'none', borderRadius: '4px', padding: '8px', cursor: 'pointer', fontSize: '10px' },
  folderHeader: { color: '#d97706', fontSize: '11px', margin: '10px 0', fontWeight: 'bold' },
  fileRow: { padding: '8px', marginBottom: '2px', display: 'flex', alignItems: 'center', background: '#f8fafc', borderRadius: '4px', border: '1px solid #f1f5f9' },
  healthBar: { display: 'flex', gap: '2px' },
  slot: { width: '10px', height: '4px', borderRadius: '1px' },
  fileActions: { display: 'flex', gap: '10px' },
  actionBtn: { background: 'none', border: 'none', color: '#2563eb', cursor: 'pointer', fontSize: '10px', fontWeight: 'bold' },
  previewCard: { background: '#f8fafc', border: '1px solid #3b82f6', borderRadius: '8px', padding: '15px' },
  previewHeader: { display: 'flex', justifyContent: 'space-between', fontSize: '10px', marginBottom: '10px', color: '#1e40af' },
  previewBody: { background: '#ffffff', padding: '10px', borderRadius: '4px', fontSize: '12px', whiteSpace: 'pre-wrap', maxHeight: '300px', overflowY: 'auto', border: '1px solid #e2e8f0', color: '#334155' },
  downloadBtn: { background: '#10b981', color: 'white', border: 'none', padding: '4px 8px', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold' },
  sidePanel: { background: '#f4f4f5', padding: '15px', borderRadius: '8px', border: '1px solid #e4e4e7' },
  logContainer: { height: '200px', overflowY: 'auto' },
  logItem: { fontSize: '10px', marginBottom: '4px' },
  tagBubble: { background: '#e0e7ff', color: '#4338ca', border: '1px solid #c7d2fe', padding: '4px 10px', borderRadius: '15px', fontSize: '11px', cursor: 'pointer' }
};

export default App;
