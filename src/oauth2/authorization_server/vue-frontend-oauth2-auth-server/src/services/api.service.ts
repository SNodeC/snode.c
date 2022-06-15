export interface LoginResponse {
    redirect_uri: string
}

class ApiService {
    private readonly API_URL: string = 'http://localhost:8082/oauth2/login'

    async login(email: string, password: string, clientId: string): Promise<LoginResponse> {
        try {
            const response = await fetch(`${this.API_URL}?client_id=${clientId}`, {
                method: 'post',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ email, password }),
            })
            return response.json()
        } catch (e: any) {
            throw new Error(e?.toString())
        }
    }
}

export const apiService = new ApiService()
