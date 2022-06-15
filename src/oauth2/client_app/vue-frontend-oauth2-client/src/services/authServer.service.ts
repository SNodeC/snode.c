interface AccessTokenResponse {
    access_token: string
    expires_in: number
    token_type: string
    refresh_token: string
}

class AuthServerService {
    private readonly apiUrl: string = 'http://localhost:8082/oauth2'

    async requestToken(authCode: string): Promise<AccessTokenResponse | null> {
        try {
            const response = await fetch(`${this.apiUrl}/token?grand_type=code&code=${authCode}`, {
                method: 'get',
                mode: 'cors',
                headers: {
                    'Origin': 'http://localhost:8081'
                }
            })
            return JSON.parse(await response.json())
        } catch (error) {
            console.error(error)
            return null
        }
    }
}

export const authServerService = new AuthServerService()
