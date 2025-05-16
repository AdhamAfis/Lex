// Main JavaScript interface for the Lex WebAssembly module
console.log('Loading app.js');

// Wait for the module to initialize
LexModule().then(function(Module) {
    console.log('Lex WebAssembly module loaded successfully');
    
    // UI Elements
    const sourceTextArea = document.getElementById('source');
    const outputDiv = document.getElementById('output');
    const languageSelect = document.getElementById('language');
    const analyzeBtn = document.getElementById('analyzeBtn');
    const clearBtn = document.getElementById('clearBtn');
    const exampleBtn = document.getElementById('exampleBtn');
    const pluginManagerBtn = document.getElementById('managePluginsBtn');
    const loadingDiv = document.getElementById('loading');
    
    // Plugin modal elements
    const pluginModal = document.getElementById('pluginModal');
    const closePluginModal = document.querySelector('.close');
    const pluginTabs = document.querySelectorAll('.tablinks');
    const pluginTabContents = document.querySelectorAll('.tabcontent');
    const savePluginBtn = document.getElementById('savePluginBtn');
    const importPluginBtn = document.getElementById('importPluginBtn');
    const pluginsList = document.getElementById('pluginsList');
    
    // Create C function wrappers
    const tokenizeString = Module.cwrap('tokenizeString', 'string', ['string', 'string']);
    const getLanguageNames = Module.cwrap('getLanguageNames', 'string', []);
    const registerLanguageConfig = Module.cwrap('registerLanguageConfig', 'boolean', ['string', 'string']);
    
    // Plugin Templates
    let pluginTemplates = {};
    
    // Load plugin templates
    async function loadPluginTemplates() {
        try {
            // Load JavaScript template
            const jsResponse = await fetch('plugin_templates/javascript_template.json');
            const jsTemplate = await jsResponse.json();
            pluginTemplates['javascript'] = jsTemplate;
            
            // Load Python template
            const pyResponse = await fetch('plugin_templates/python_template.json');
            const pyTemplate = await pyResponse.json();
            pluginTemplates['python'] = pyTemplate;
            
            // Load Ruby template
            const rubyResponse = await fetch('plugin_templates/ruby_template.json');
            const rubyTemplate = await rubyResponse.json();
            pluginTemplates['ruby'] = rubyTemplate;
            
            console.log('Plugin templates loaded successfully');
        } catch(e) {
            console.error('Error loading plugin templates:', e);
        }
    }
    
    // Custom Language Plugins stored in localStorage
    const STORAGE_KEY = 'lex_custom_plugins';
    
    // Load custom plugins from localStorage
    function loadCustomPlugins() {
        const storedPlugins = localStorage.getItem(STORAGE_KEY);
        if (storedPlugins) {
            try {
                return JSON.parse(storedPlugins);
            } catch(e) {
                console.error('Error parsing stored plugins:', e);
                return {};
            }
        }
        return {};
    }
    
    // Save custom plugins to localStorage
    function saveCustomPlugins(plugins) {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(plugins));
    }
    
    // Register plugins with the WASM module
    function registerPlugins(plugins) {
        Object.entries(plugins).forEach(([id, config]) => {
            try {
                console.log(`Registering plugin: ${id}`);
                const success = registerLanguageConfig(id, JSON.stringify(config));
                if (!success) {
                    console.error(`Failed to register plugin: ${id}`);
                }
            } catch(e) {
                console.error(`Error registering plugin ${id}:`, e);
            }
        });
    }
    
    // Populate language dropdown
    function populateLanguages() {
        try {
            console.log('Getting language names');
            const languagesJson = getLanguageNames();
            console.log('Language names received:', languagesJson);
            const languages = JSON.parse(languagesJson);
            
            // Clear existing options
            languageSelect.innerHTML = '';
            
            // Add languages from the module
            languages.forEach(lang => {
                const option = document.createElement('option');
                option.value = lang.id;
                option.textContent = lang.name;
                languageSelect.appendChild(option);
            });
        } catch(e) {
            console.error('Error loading languages:', e);
            // Fallback to hardcoded languages
            const fallbackLanguages = [
                { id: 'cpp', name: 'C++' },
                { id: 'js', name: 'JavaScript' },
                { id: 'python', name: 'Python' },
                { id: 'java', name: 'Java' },
                { id: 'c', name: 'C' }
            ];
            
            languageSelect.innerHTML = '';
            fallbackLanguages.forEach(lang => {
                const option = document.createElement('option');
                option.value = lang.id;
                option.textContent = lang.name;
                languageSelect.appendChild(option);
            });
        }
    }
    
    // Process tokens and display them
    function displayTokens(tokensJson, executionTime) {
        try {
            console.log('Parsing tokens JSON');
            const tokens = JSON.parse(tokensJson);
            
            // Hide loading indicator
            loadingDiv.style.display = 'none';
            
            // Clear previous output
            outputDiv.innerHTML = '';
            
            // Create a container for execution stats
            const statsContainer = document.createElement('div');
            statsContainer.className = 'execution-stats';
            
            // Add execution time 
            const timeDisplay = document.createElement('div');
            timeDisplay.className = 'execution-time';
            timeDisplay.innerHTML = `<span>Execution time:</span> ${executionTime} ms`;
            statsContainer.appendChild(timeDisplay);
            
            // Add token count
            const tokenCountDiv = document.createElement('div');
            tokenCountDiv.className = 'token-count';
            tokenCountDiv.textContent = `Total tokens: ${tokens.tokens.length}`;
            statsContainer.appendChild(tokenCountDiv);
            
            // Add stats container to output
            outputDiv.appendChild(statsContainer);
            
            // Display error if present
            if (tokens.error) {
                const errorContainer = document.createElement('div');
                errorContainer.className = 'error-container';
                errorContainer.innerHTML = `
                    <div class="error-title">Error during lexical analysis:</div>
                    <div class="error-message">${escapeHtml(tokens.error)}</div>
                `;
                outputDiv.appendChild(errorContainer);
            }
            
            // Display each token with appropriate styling
            tokens.tokens.forEach(token => {
                const tokenDiv = document.createElement('div');
                tokenDiv.className = `token ${token.type.toLowerCase()}`;
                
                let locationInfo = '';
                if (token.line && token.column) {
                    locationInfo = `<span class="token-location">line ${token.line}, col ${token.column}</span>`;
                }
                
                tokenDiv.innerHTML = `
                    <span class="token-type">${token.type}</span>
                    <span class="token-lexeme">"${escapeHtml(token.lexeme)}"</span>
                    ${locationInfo}
                `;
                outputDiv.appendChild(tokenDiv);
            });
        } catch(e) {
            console.error('Error parsing tokens:', e);
            loadingDiv.style.display = 'none';
            outputDiv.innerHTML = `
                <div class="error-container">
                    <div class="error-title">Error parsing output:</div>
                    <div class="error-message">${e.message}</div>
                </div>
            `;
        }
    }
    
    // Helper function to escape HTML
    function escapeHtml(unsafe) {
        return unsafe
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#039;");
    }
    
    // Analyze the source code
    function analyzeCode() {
        console.log('Analyze button clicked');
        const sourceCode = sourceTextArea.value;
        if (!sourceCode.trim()) {
            outputDiv.innerHTML = '<div class="error-container"><div class="error-message">Please enter some code to analyze</div></div>';
            return;
        }
        
        const selectedLanguage = languageSelect.value;
        console.log('Analyzing code for language:', selectedLanguage);
        
        // Show loading indicator
        loadingDiv.style.display = 'flex';
        outputDiv.innerHTML = '';
        
        // Use setTimeout to ensure the UI updates before processing
        setTimeout(() => {
            try {
                console.log('Calling tokenizeString');
                
                // Start timing
                const startTime = performance.now();
                
                // Perform the tokenization
                const result = tokenizeString(sourceCode, selectedLanguage);
                
                // End timing
                const endTime = performance.now();
                const executionTime = Math.round(endTime - startTime);
                
                console.log(`Tokenization completed in ${executionTime} ms`);
                
                // Display results with timing information
                displayTokens(result, executionTime);
            } catch(e) {
                console.error('Error during tokenization:', e);
                loadingDiv.style.display = 'none';
                outputDiv.innerHTML = `
                    <div class="error-container">
                        <div class="error-title">Error during tokenization:</div>
                        <div class="error-message">${e.message}</div>
                    </div>
                `;
            }
        }, 50);
    }
    
    // Clear the inputs and outputs
    function clearAll() {
        sourceTextArea.value = '';
        outputDiv.innerHTML = '';
    }
    
    // Add example code based on selected language
    function addExampleCode() {
        const lang = languageSelect.value;
        let example = '';
        
        if (lang === 'cpp' || lang === 'c++') {
            example = `#include <iostream>\n\nint main() {\n    std::cout << "Hello, World!" << std::endl;\n    return 0;\n}`;
        } else if (lang === 'js' || lang === 'javascript') {
            example = `function greet(name) {\n    console.log("Hello, " + name + "!");\n    return true;\n}\n\ngreet("World");`;
        } else if (lang === 'java') {
            example = `public class HelloWorld {\n    public static void main(String[] args) {\n        System.out.println("Hello, World!");\n    }\n}`;
        } else if (lang === 'python' || lang === 'py') {
            example = `def greet(name):\n    print(f"Hello, {name}!")\n    return True\n\ngreet("World")`;
        } else if (lang === 'c') {
            example = `#include <stdio.h>\n\nint main() {\n    printf("Hello, World!\\n");\n    return 0;\n}`;
        } else {
            example = `// Example code for ${lang}\nfunction example() {\n    return "Hello, World!";\n}`;
        }
        
        sourceTextArea.value = example;
    }
    
    // Tab switcher function (matching the onclick in HTML)
    window.openTab = function(event, tabName) {
        // Hide all tab contents
        const tabcontents = document.getElementsByClassName("tabcontent");
        for (let i = 0; i < tabcontents.length; i++) {
            tabcontents[i].style.display = "none";
        }
        
        // Remove active class from all tabs
        const tablinks = document.getElementsByClassName("tablinks");
        for (let i = 0; i < tablinks.length; i++) {
            tablinks[i].className = tablinks[i].className.replace(" active", "");
        }
        
        // Show the selected tab and add active class
        document.getElementById(tabName).style.display = "block";
        event.currentTarget.className += " active";
    };
    
    // Refresh the plugins list in the manager
    function refreshPluginsList() {
        const customPlugins = loadCustomPlugins();
        
        // Clear existing list
        pluginsList.innerHTML = '';
        
        if (Object.keys(customPlugins).length === 0) {
            pluginsList.innerHTML = '<div class="no-plugins">No custom plugins yet. Create one using the "Create New Plugin" tab.</div>';
            return;
        }
        
        // Add each plugin to the list
        Object.entries(customPlugins).forEach(([id, config]) => {
            const pluginItem = document.createElement('div');
            pluginItem.className = 'plugin-item';
            
            pluginItem.innerHTML = `
                <div class="plugin-info">
                    <h3>${config.name}</h3>
                    <p>${id}</p>
                </div>
                <div class="plugin-actions">
                    <button class="export-plugin" data-id="${id}">Export</button>
                    <button class="delete-plugin" data-id="${id}">Delete</button>
                </div>
            `;
            
            pluginsList.appendChild(pluginItem);
        });
        
        // Add event listeners to the buttons
        document.querySelectorAll('.export-plugin').forEach(button => {
            button.addEventListener('click', (e) => {
                const id = e.target.getAttribute('data-id');
                exportPlugin(id);
            });
        });
        
        document.querySelectorAll('.delete-plugin').forEach(button => {
            button.addEventListener('click', (e) => {
                const id = e.target.getAttribute('data-id');
                deletePlugin(id);
            });
        });
    }
    
    // Export a plugin to a JSON file
    function exportPlugin(id) {
        const customPlugins = loadCustomPlugins();
        if (!customPlugins[id]) {
            alert(`Plugin ${id} not found`);
            return;
        }
        
        const pluginJson = JSON.stringify(customPlugins[id], null, 2);
        const blob = new Blob([pluginJson], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        
        const a = document.createElement('a');
        a.href = url;
        a.download = `${id}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }
    
    // Delete a plugin
    function deletePlugin(id) {
        if (!confirm(`Are you sure you want to delete the plugin "${id}"?`)) {
            return;
        }
        
        const customPlugins = loadCustomPlugins();
        if (customPlugins[id]) {
            delete customPlugins[id];
            saveCustomPlugins(customPlugins);
            refreshPluginsList();
            populateLanguages(); // Refresh language dropdown
        }
    }
    
    // Handle saving a new plugin
    function savePlugin() {
        const pluginName = document.getElementById('pluginName').value.trim();
        const pluginId = document.getElementById('pluginId').value.trim().toLowerCase();
        let pluginConfig;
        
        try {
            // Get config from textarea or template
            const configText = document.getElementById('pluginConfig').value.trim();
            if (configText) {
                pluginConfig = JSON.parse(configText);
            } else {
                // If textarea is empty, check if a template is selected
                const templateType = document.getElementById('templateType').value;
                if (templateType && pluginTemplates[templateType]) {
                    pluginConfig = JSON.parse(JSON.stringify(pluginTemplates[templateType])); // Deep clone
                } else {
                    throw new Error('Please provide a configuration or select a template');
                }
            }
        } catch (e) {
            alert('Invalid JSON configuration: ' + e.message);
            return;
        }
        
        // Validate inputs
        if (!pluginName || !pluginId) {
            alert('Please fill in both name and ID fields');
            return;
        }
        
        // Check if ID already exists
        const customPlugins = loadCustomPlugins();
        if (customPlugins[pluginId]) {
            if (!confirm(`A plugin with ID "${pluginId}" already exists. Do you want to overwrite it?`)) {
                return;
            }
        }
        
        // Add name to config if not present
        pluginConfig.name = pluginName;
        
        // Save the plugin
        customPlugins[pluginId] = pluginConfig;
        saveCustomPlugins(customPlugins);
        
        // Register with WASM
        try {
            const success = registerLanguageConfig(pluginId, JSON.stringify(pluginConfig));
            if (!success) {
                console.error(`Failed to register plugin: ${pluginId}`);
            }
        } catch(e) {
            console.error(`Error registering plugin ${pluginId}:`, e);
        }
        
        // Reset form and refresh views
        document.getElementById('pluginName').value = '';
        document.getElementById('pluginId').value = '';
        document.getElementById('pluginConfig').value = '';
        document.getElementById('templateType').selectedIndex = 0;
        refreshPluginsList();
        populateLanguages(); // Refresh language dropdown
        
        // Show success message and switch to installed plugins tab
        alert(`Plugin "${pluginName}" saved successfully!`);
        document.querySelector('.tablinks[onclick*="installedPlugins"]').click();
    }
    
    // Handle plugin import
    function importPlugin() {
        const fileInput = document.getElementById('pluginFile');
        if (!fileInput.files.length) {
            alert('Please select a file to import');
            return;
        }
        
        const file = fileInput.files[0];
        const reader = new FileReader();
        
        reader.onload = function(e) {
            try {
                const pluginConfig = JSON.parse(e.target.result);
                
                // Validate basic structure
                if (!pluginConfig.name) {
                    alert('Invalid plugin file: Missing name property');
                    return;
                }
                
                // Get plugin ID from filename or prompt user
                let pluginId = file.name.replace(/\.json$/, '').toLowerCase();
                const customPlugins = loadCustomPlugins();
                
                if (customPlugins[pluginId]) {
                    const customId = prompt(`A plugin with ID "${pluginId}" already exists. Please enter a new ID:`, '');
                    if (!customId) return;
                    pluginId = customId.trim().toLowerCase();
                    
                    if (customPlugins[pluginId]) {
                        alert(`A plugin with ID "${pluginId}" also exists. Import cancelled.`);
                        return;
                    }
                }
                
                // Save the imported plugin
                customPlugins[pluginId] = pluginConfig;
                saveCustomPlugins(customPlugins);
                
                // Register with WASM
                try {
                    const success = registerLanguageConfig(pluginId, JSON.stringify(pluginConfig));
                    if (!success) {
                        console.error(`Failed to register plugin: ${pluginId}`);
                    }
                } catch(e) {
                    console.error(`Error registering plugin ${pluginId}:`, e);
                }
                
                // Reset form and refresh views
                fileInput.value = '';
                refreshPluginsList();
                populateLanguages(); // Refresh language dropdown
                
                // Show success message and switch to installed plugins tab
                alert(`Plugin "${pluginConfig.name}" imported successfully as "${pluginId}"!`);
                document.querySelector('.tablinks[onclick*="installedPlugins"]').click();
                
            } catch(e) {
                console.error('Error importing plugin:', e);
                alert(`Error importing plugin: ${e.message}`);
            }
        };
        
        reader.readAsText(file);
    }
    
    // Handle template selection
    function handleTemplateSelection() {
        const templateType = document.getElementById('templateType').value;
        const configTextarea = document.getElementById('pluginConfig');
        
        if (templateType && pluginTemplates[templateType]) {
            // Format the JSON with nice indentation for display
            configTextarea.value = JSON.stringify(pluginTemplates[templateType], null, 2);
        } else {
            configTextarea.value = '';
        }
    }

    // Handle file selection display
    function handleFileSelection(e) {
        const fileInput = e.target;
        const filePreview = document.getElementById('filePreview');
        const selectedFileName = document.getElementById('selectedFileName');
        
        if (fileInput.files.length > 0) {
            const file = fileInput.files[0];
            selectedFileName.textContent = file.name;
            filePreview.style.display = 'block';
        } else {
            filePreview.style.display = 'none';
        }
    }
    
    // Handle file removal
    function handleFileRemoval() {
        const fileInput = document.getElementById('pluginFile');
        const filePreview = document.getElementById('filePreview');
        
        fileInput.value = '';
        filePreview.style.display = 'none';
    }
    
    // Set up event listeners
    function setupEventListeners() {
        // Main functionality buttons
        analyzeBtn.addEventListener('click', analyzeCode);
        clearBtn.addEventListener('click', clearAll);
        exampleBtn.addEventListener('click', addExampleCode);
        
        // Plugin manager
        pluginManagerBtn.addEventListener('click', function() {
            refreshPluginsList();
            pluginModal.style.display = 'block';
        });
        
        closePluginModal.addEventListener('click', function() {
            pluginModal.style.display = 'none';
        });
        
        // Plugin form buttons
        savePluginBtn.addEventListener('click', savePlugin);
        importPluginBtn.addEventListener('click', importPlugin);
        
        // Template selection
        const templateSelect = document.getElementById('templateType');
        if (templateSelect) {
            templateSelect.addEventListener('change', handleTemplateSelection);
        }
        
        // File upload handling
        const fileInput = document.getElementById('pluginFile');
        const removeFileBtn = document.getElementById('removeFileBtn');
        
        if (fileInput) {
            fileInput.addEventListener('change', handleFileSelection);
        }
        
        if (removeFileBtn) {
            removeFileBtn.addEventListener('click', handleFileRemoval);
        }
        
        // Close modal when clicking outside
        window.addEventListener('click', function(e) {
            if (e.target === pluginModal) {
                pluginModal.style.display = 'none';
            }
        });
        
        // Handle keyboard shortcuts
        document.addEventListener('keydown', function(e) {
            // Close modal with Escape key
            if (e.key === 'Escape' && pluginModal.style.display === 'block') {
                pluginModal.style.display = 'none';
            }
            
            // Analyze with Ctrl+Enter or Cmd+Enter
            if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
                if (document.activeElement === sourceTextArea) {
                    e.preventDefault();
                    analyzeCode();
                }
            }
        });
    }
    
    // Initialize the application
    async function init() {
        try {
            console.log('Initializing application');
            
            // Load plugin templates first
            await loadPluginTemplates();
            
            // Register custom plugins
            const customPlugins = loadCustomPlugins();
            registerPlugins(customPlugins);
            
            // Populate languages after registering plugins
            populateLanguages();
            
            // Set up event listeners
            setupEventListeners();
            
            console.log('Application initialized successfully');
        } catch(e) {
            console.error('Error initializing application:', e);
        }
    }
    
    // Start the application
    init();
    
}).catch(function(error) {
    console.error('Error loading Lex module:', error);
    if (document.getElementById('output')) {
        document.getElementById('output').innerHTML = 
            `<div class="token error">Error loading the Lex analyzer: ${error.message}</div>`;
    }
});

// Global error handler
window.addEventListener('error', function(event) {
    console.error('Global error:', event.message);
    if (document.getElementById('output')) {
        document.getElementById('output').innerHTML = 
            `<div class="token error">Error: ${event.message}</div>`;
    }
}); 