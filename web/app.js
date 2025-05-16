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
    const loadingDiv = document.getElementById('loading');
    
    // Create C function wrappers
    const tokenizeString = Module.cwrap('tokenizeString', 'string', ['string', 'string']);
    const getLanguageNames = Module.cwrap('getLanguageNames', 'string', []);

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

    // Set up event listeners
    function setupEventListeners() {
        analyzeBtn.addEventListener('click', analyzeCode);
        clearBtn.addEventListener('click', clearAll);
        exampleBtn.addEventListener('click', addExampleCode);
        // All plugin-related event listeners removed
    }

    // Initialize the application
    function init() {
        try {
            console.log('Initializing application');
            // Populate languages
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