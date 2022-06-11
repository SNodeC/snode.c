class ApiService {
    private readonly API_URL: string = '/login'

    async login(email: string, password: string): Promise<unknown> {
        try {
            const response = await fetch(this.API_URL, {
                method: 'post',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ email, password }),
            })
            return response.json()
        } catch (e: any) {
            throw new Error(e?.toString());
        }
    }
}

export const apiService = new ApiService()
