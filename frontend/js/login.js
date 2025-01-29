const loginTab = document.getElementById('loginTab');
const signupTab = document.getElementById('signupTab');
const loginForm = document.getElementById('loginForm');
const signupForm = document.getElementById('signupForm');
const errorDiv = document.getElementById('errorMessage');

loginTab.addEventListener('click', () => {
    loginForm.classList.remove('hidden');
    signupForm.classList.add('hidden');
    loginTab.classList.add('text-blue-500');
    loginTab.classList.remove('text-gray-500');
    signupTab.classList.add('text-gray-500');
    signupTab.classList.remove('text-blue-500');
    errorDiv.classList.add('hidden');
});

signupTab.addEventListener('click', () => {
    signupForm.classList.remove('hidden');
    loginForm.classList.add('hidden');
    signupTab.classList.add('text-blue-500');
    signupTab.classList.remove('text-gray-500');
    loginTab.classList.add('text-gray-500');
    loginTab.classList.remove('text-blue-500');
    errorDiv.classList.add('hidden');
});

document.getElementById('loginForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    
    try {
        const response = await fetch('http://localhost:8080/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });
        
        const data = await response.json();
        if (data.success) {
            localStorage.setItem('credentials', JSON.stringify({ username, password }));
            if (data.is_admin) {
                window.location.href = '/admin.html';
            } else {
                document.body.innerHTML = '<div class="text-2xl font-bold text-green-600">Welcome!</div>';
            }
        } else {
            errorDiv.textContent = 'Invalid credentials';
            errorDiv.classList.remove('hidden');
        }
    } catch (err) {
        errorDiv.textContent = 'Server error';
        errorDiv.classList.remove('hidden');
    }
});

signupForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    const username = document.getElementById('signupUsername').value;
    const password = document.getElementById('signupPassword').value;

    try {
        const response = await fetch('http://localhost:8080/signup', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
            errorDiv.textContent = 'Signup successful! Please login.';
            errorDiv.classList.remove('text-red-500');
            errorDiv.classList.add('text-green-500');
            errorDiv.classList.remove('hidden');
            loginTab.click();
        } else {
            errorDiv.textContent = data.message || 'Signup failed';
            errorDiv.classList.add('text-red-500');
            errorDiv.classList.remove('text-green-500');
            errorDiv.classList.remove('hidden');
        }
    } catch (err) {
        errorDiv.textContent = 'Server error';
        errorDiv.classList.add('text-red-500');
        errorDiv.classList.remove('hidden');
    }
});
