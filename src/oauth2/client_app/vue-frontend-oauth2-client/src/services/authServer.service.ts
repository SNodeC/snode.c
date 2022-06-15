interface AccessTokenResponse {
    access_token: string
    expires_in: number
    token_type: string
    refresh_token: string
}

class AuthServerService {
    private readonly apiUrl: string = 'http://localhost:8082/oauth2'

    async requestToken(authCode: string, clientId: string): Promise<AccessTokenResponse | null> {
        const response = await fetch(
            `${this.apiUrl}/token?grant_type=authorization_code&code=${authCode}&client_id=${clientId}`,
            {
                method: 'get',
                mode: 'cors',
                headers: {
                    'Origin': 'http://localhost:8081'
                }
            })
        if (response.status === 200) {
            return response.json()
        }
        throw new Error(`Server responded with status code '${response.status}'`)
    }
}

export const authServerService = new AuthServerService()
