async function login() {
    const email = document.getElementById("input-email").value;
    const password = document.getElementById("input-password").value;
    const client_id = "c1234";
    const response = await fetch("http://localhost:8082/login", {
        method: 'POST', // *GET, POST, PUT, DELETE, etc.
        headers: {
            'Content-Type': 'application/json',
            // 'Access-Control-Allow-Origin': '*',
        },
        mode: 'cors',
        body: JSON.stringify({
            email,
            password,
            client_id,
        }),
    });
    window.alert(await response.json());
}
