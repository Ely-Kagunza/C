// ========================== CONFIGURATION ==========================

const API_BASE_URL = "http://127.0.0.1:8080";
const HEALTH_CHECK_INTERVAL = 5000; // 5 seconds

// ========================== STATE ==========================

let currentData = [];
let healthCheckInterval;

// ========================== INITIALIZATION ==========================

document.addEventListener('DOMContentLoaded', () => {
    initializeEventListeners();
    checkServerHealth();
    healthCheckInterval = setInterval(checkServerHealth, HEALTH_CHECK_INTERVAL);
});

// ========================== EVENT LISTENERS ==========================

function initializeEventListeners() {
    // Tab navigation
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', switchTab);
    });

    // View tab
    document.getElementById('loadAllBtn').addEventListener('click', loadAllPeople);
    document.getElementById('refreshBtn').addEventListener('click', loadAllPeople);

    // Add tab
    document.getElementById('addForm').addEventListener('submit', HandleAddPerson);

    // Search tab
    document.getElementById('searchBtn').addEventListener('click', HandleSearch);
    document.getElementById('searchId').addEventListener('keypress', e => {
        if (e.key === 'Enter') handleSearch();
    });

    // Range query tab
    document.getElementById('ageRangeBtn').addEventListener('click', HandleAgeRange);
    document.getElementById('salaryRangeBtn').addEventListener('click', HandleSalaryRange);

    // Modal
    document.querySelector('.close').addEventListener('click', closeModal);
    window.addEventListener('click', e => {
        if (e.target.id === 'editForm') closeModal();
    });

    document.getElementById('editForm').addEventListener('submit', HandleUpdatePerson);
}

// ========================== TAB MANAGEMENT ==========================

function switchTab(e) {
    const tabName = e.target.dataset.tab;

    // Remove active class from all buttons and contents
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

    // Add active class to clicked button and corresponding content
    e.target.classList.add('active');
    document.getElementById(`${tabName}-tab`).classList.add('active');
}

// ========================== SERVER HEALTH CHECK ==========================

async function checkServerHealth() {
    try {
        const response = await fetch(`${API_BASE_URL}/api/health`);
        const data = await response.json();

        // Update status indicator
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');
        const recordCount = document.getElementById('recordCount');

        statusIndicator.classList.add('online');
        statusText.textContent = `✓ Online`;
        recordCount.textContent = `Records: ${data.records}`;
    } catch (error) {
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');

        statusIndicator.classList.remove('online');
        statusIndicator.classList.add('offline');
        statusText.textContent = `✗ Offline`;
    }
}

// ========================== VIEW ALL PEOPLE ==========================

async function loadAllPeople() {
    try {
        showMessage('Loading...', 'info', 'addMessage');

        const response = await fetch(`${API_BASE_URL}/api/people`);

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();

        if (!Array.isArray(data)) {
            throw new Error('Invalid response format');
        }

        currentData = data;
        renderTable(data);
        showMessage(`Loaded ${data.length} records`, 'success', 'addMessage');
    } catch (error) {
        showMessage(`Error loading data: ${error.message}`, 'error', 'addMessage');
        console.error('Error:', error);
    }
}

function renderTable(data) {
    const tableBody = document.getElementById('tableBody');

    if (!data || data.length === 0) {
        tableBody.innerHTML = '<tr class="empty-row"><td colspan="5">No records found</td></tr>';
        return;
    }

    tableBody.innerHTML = data.map(person => `
        <tr>
            <td>${person.id}</td>
            <td>${escapeHtml(person.name)}</td>
            <td>${person.age}</td>
            <td>$${formatSalary(person.salary)}</td>
            <td>
                <div class="action-buttons">
                    <button class="btn btn-warning btn-small" onclick="openEditModal(${person.id}, '${escapeHtml(person.name)}', ${person.age}, ${person.salary})">Edit</button>
                    <button class="btn btn-danger btn-small" onclick="handleDelete(${person.id})">Delete</button>
                </div>
            </td>
        </tr>
    `).join('');
}

// ========================== ADD NEW PERSON ==========================

async function HandleAddPerson(e) {
    e.preventDefault();

    const id = parseInt(document.getElementById('addId').value);
    const name = document.getElementById('addName').value.trim();
    const age = parseInt(document.getElementById('addAge').value);
    const salary = parseFloat(document.getElementById('addSalary').value);

    if (!id || !name || !age || !salary) {
        showMessage('Please fill in all fields', 'error', 'addMessage');
        return;
    }

    try {
        showMessage('Adding person...', 'info', 'addMessage');

        const response = await fetch(`${API_BASE_URL}/api/people`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, name, age, salary }),
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.message || `HTTP error! status: ${response.status}`);
        }

        const result = await response.json();
        showMessage(`✓ ${name} added successfully!`, 'success', 'addMessage');

        // Reset form
        document.getElementById('addForm').reset();

        // Refresh table
        setTimeout(loadAllPeople, 500);
    } catch (error) {
        showMessage(`Error: ${error.message}`, 'error', 'addMessage');
        console.error('Error:', error);
    }
}

// ========================== SEARCH BY ID ==========================

async function HandleSearch() {
    const id = parseInt(document.getElementById('searchId').value);

    if (!id) {
        showMessage('Please enter a valid ID', 'error', 'searchMessage');
        return;
    }

    try {
        showMessage('Searching...', 'info', 'searchMessage');

        const response = await fetch(`${API_BASE_URL}/api/people/${id}`);
        const data = await response.json();

        if (data.error) {
            throw new Error(data.error);
        }

        renderSearchResult(data);
        showMessage('✓ Found!', 'success', 'searchMessage');
    } catch (error) {
        document.getElementById('searchResult').classList.remove('show');
        showMessage(`Person not found: ${error.message}`, 'error', 'searchMessage');
        console.error('Error:', error);
    }
}

function renderSearchResult(person) {
    const resultDiv = document.getElementById('searchResult');
    resultDiv.innerHTML = `
        <div class="person-card">
            <div class="person-field">
                <span class="person-label">ID:</span>
                <span class="person-value">${person.id}</span>
            </div>
            <div class="person-field">
                <span class="person-label">Name:</span>
                <span class="person-value">${escapeHtml(person.name)}</span>
            </div>
            <div class="person-field">
                <span class="person-label">Age:</span>
                <span class="person-value">${person.age}</span>
            </div>
            <div class="person-field">
                <span class="person-label">Salary:</span>
                <span class="person-value">$${formatSalary(person.salary)}</span>
            </div>
            <div style="margin-top: 15px; display: flex; gap: 10px;">
                <button class="btn btn-warning btn-small" 
                    onclick="openEditModal(${person.id}, '${escapeHtml(person.name)}', ${person.age}, ${person.salary})">
                    Edit
                </button>
                <button class="btn btn-danger btn-small" onclick="handleDelete(${person.id})">Delete</button>
            </div>
        </div>
    `;
    resultDiv.classList.add('show');
}

// ========================== EDIT PERSON ==========================

function openEditModal(id, name, age, salary) {
    document.getElementById('editId').value = id;
    document.getElementById('editName').value = name;
    document.getElementById('editAge').value = age;
    document.getElementById('editSalary').value = salary;
    document.getElementById('editModal').classList.add('show');
}

function closeModal() {
    document.getElementById('editModal').classList.remove('show');
}

async function HandleUpdatePerson(e) {
    e.preventDefault();

    const id = parseInt(document.getElementById('editId').value);
    const name = document.getElementById('editName').value.trim();
    const age = parseInt(document.getElementById('editAge').value);
    const salary = parseFloat(document.getElementById('editSalary').value);

    if (!name || !age || !salary) {
        alert('Please fill in all fields');
        return;
    }

    try {
        const response = await fetch(`${API_BASE_URL}/api/people/${id}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, age, salary }),
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.message || `HTTP error! status: ${response.status}`);
        }

        showMessage(`✓ ${name} updated successfully!`, 'success', 'editMessage');
        closeModal();
        setTimeout(loadAllPeople, 500);
    } catch (error) {
        alert(`Error updating person: ${error.message}`);
        console.error('Error:', error);
    }
}

// ========================== DELETE PERSON ==========================

async function handleDelete(id) {
    if (!confirm(`Are you sure you want to delete person ${id}?`)) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE_URL}/api/people/${id}`, {
            method: 'DELETE'
        });

        if (!response.ok) {
            const error = await response.json();
            throw new Error(error.message || `HTTP error! status: ${response.status}`);
        }

        showMessage(`✓ Person ${id} deleted successfully!`, 'success', 'deleteMessage');
        setTimeout(loadAllPeople, 500);
    } catch (error) {
        showMessage(`Error deleting person: ${error.message}`, 'error', 'deleteMessage');
        console.error('Error:', error);
    }
}

// ========================== RANGE QUERIES ==========================

async function HandleAgeRange() {
    const minAge = parseInt(document.getElementById('ageMin').value);
    const maxAge = parseInt(document.getElementById('ageMax').value);

    if (!minAge || !maxAge || minAge > maxAge) {
        showMessage('Please enter valid age range', 'error', 'rangeMessage');
        return;
    }

    try {
        const response = await fetch(
            `${API_BASE_URL}/api/people/age-range?minAge=${minAge}&maxAge=${maxAge}`
        );
        const data = await response.json();

        renderRangeResult(data, 'ageRangeResult');
        showMessage(`✓ Found ${data.length} records`, 'success', 'ageRangeResult');
    } catch (error) {
        showMessage(`Error: ${error.message}`, 'error', 'ageRangeResult');
        console.error('Error:', error);
    }
}

async function HandleSalaryRange() {
    const minSalary = parseFloat(document.getElementById('salaryMin').value);
    const maxSalary = parseFloat(document.getElementById('salaryMax').value);

    if (!minSalary || !maxSalary || minSalary > maxSalary) {
        showMessage('Please enter valid salary range', 'error', 'rangeMessage');
        return;
    }

    try {
        const response = await fetch(
            `${API_BASE_URL}/api/people?salary_min=${minSalary}&salary_max=${maxSalary}`
        );
        const data = await response.json();

        renderRangeResult(data, 'salaryRangeResult');
        showMessage(`✓ Found ${data.length} records`, 'success', 'salaryRangeResult');
    } catch (error) {
        showMessage(`Error: ${error.message}`, 'error', 'salaryRangeResult');
        console.error('Error:', error);
    }
}

function renderRangeResult(data, elementId) {
    const resultDiv = document.getElementById(elementId);

    if (!data || data.length === 0) {
        resultDiv.innerHTML = '<p style="text-align: center; color: var(--secondary-color);">No records found</p>';
        resultDiv.classList.add('show');
        return;
    }

    const tableHtml = `
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Age</th>
                    <th>Salary</th>
                </tr>
            </thead>
            <tbody>
                ${data.map(person => `
                    <tr>
                        <td>${person.id}</td>
                        <td>${escapeHtml(person.name)}</td>
                        <td>${person.age}</td>
                        <td>$${formatSalary(person.salary)}</td>
                    </tr>
                `).join('')}
            </tbody>
        </table>
    `;

    resultDiv.innerHTML = tableHtml;
    resultDiv.classList.add('show');
}

// ========================== UTILITY FUNCTIONS ==========================

function showMessage(text, type, elementId) {
    const msgElement = document.getElementById(elementId) || document.querySelector('.message');
    msgElement.textContent = text;
    msgElement.className = `message show ${type}`;

    if (type !== 'info') {
        setTimeout(() => {
            msgElement.classList.remove('show');
        }, 5000);
    }
}

function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;',
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

function formatSalary(salary) {
    return new Intl.NumberFormat('en-US', {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2
    }).format(salary);
}